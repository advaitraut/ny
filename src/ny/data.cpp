// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/data.hpp>
#include <ny/imageData.hpp>
#include <nytl/utf.hpp>
#include <sstream>

namespace ny
{

namespace
{

bool sameBeginning(nytl::SizedStringParam a, nytl::SizedStringParam b)
{
	return !std::strncmp(a, b, std::min(a.size(), b.size()));
}

}

// standard data formats
const DataFormat DataFormat::none {};
const DataFormat DataFormat::raw {"application/octet-stream",
	{"application/binary", "applicatoin/unknown", "raw", "binary", "buffer", "unknown"}};
const DataFormat DataFormat::text {"text/plain", {"text", "string",
	"unicode", "utf8", "STRING", "TEXT", "UTF8_STRING", "UNICODETEXT"}};
const DataFormat DataFormat::uriList {"text/uri-list", {"uriList"}};
const DataFormat DataFormat::imageData {"image/x-ny-data", {"imageData", "ny::ImageData"}};

std::vector<uint8_t> serialize(const ImageData& image)
{
	std::vector<uint8_t> ret;

	//store real stride
	auto stride = image.stride;
	if(!stride) stride = image.size.x * imageDataFormatSize(image.format);

	//width, height, format, stride stored as uint32_t
	ret.resize(4 * 4);
	reinterpret_cast<uint32_t&>(ret[0]) = image.size.x;
	reinterpret_cast<uint32_t&>(ret[4]) = image.size.y;
	reinterpret_cast<uint32_t&>(ret[8]) = static_cast<uint32_t>(image.format);
	reinterpret_cast<uint32_t&>(ret[12]) = stride;

	//total size of data
	auto dataSize = stride * image.size.y;
	ret.resize(ret.size() + dataSize);
	std::memcpy(&ret[16], image.data, dataSize);

	return ret;
}

OwnedImageData deserializeImageData(nytl::Range<uint8_t> buffer)
{
	OwnedImageData image;
	if(buffer.size() < 16) return {}; //invalid header

	image.size.x = reinterpret_cast<const uint32_t&>(buffer[0]);
	image.size.y = reinterpret_cast<const uint32_t&>(buffer[4]);
	image.format = static_cast<ImageDataFormat>(reinterpret_cast<const uint32_t&>(buffer[8]));
	image.stride = reinterpret_cast<const uint32_t&>(buffer[12]);

	//real stride
	auto stride = image.stride;
	if(!stride) stride = image.size.x * imageDataFormatSize(image.format);

	auto dataSize = stride * image.size.y;
	if(buffer.size() - 16 < dataSize) return {}; //invalid data size

	image.data = std::make_unique<uint8_t[]>(dataSize);
	std::memcpy(image.data.get(), &buffer[16], dataSize);

	return image;
}

//see roughly: https://tools.ietf.org/html/rfc3986

std::string encodeUriList(const std::vector<std::string>& uris)
{
	std::string ret;
	ret.reserve(uris.size() * 10);

	//first put the uris together and escape special chars
	for(auto& uri : uris)
	{
		//correct utf8 parsing
		for(auto i = 0u; i < nytl::charCount(uri); ++i)
		{
			auto chars = nytl::nth(uri, i);
			auto cint = reinterpret_cast<uint32_t&>(*chars.data());

			//the chars that should not be encoded in uris (besides alphanum values)
			std::string special = ":/?#[]@!$&'()*+,;=-_~.";
			if(cint <= 255 && (std::isalnum(cint) || special.find(cint) != std::string::npos))
			{
				ret.append(chars.data());
			}
			else
			{
				auto last = 0u;
				for(auto i = 0u; i < chars.size(); ++i) if(chars[i]) last = i;
				for(auto i = 0u; i <= last; ++i)
				{
					unsigned int ci = static_cast<unsigned char>(chars[i]);
					std::stringstream sstream;
					sstream << std::hex << ci;
					ret.append("%");
					ret.append(sstream.str());
				}
			}
		}

		//note that the uri spec sperates lines with "\r\n"
		ret.append("\r\n");
	}

	return ret;
}

std::vector<std::string> decodeUriList(const std::string& escaped, bool removeComments)
{
	std::string uris;
	uris.reserve(escaped.size());

	//copy <escaped> into (non-const) <uris> into this loop, but replace
	//the escape codes on the run
	for(auto i = 0u; i < escaped.size(); ++i)
	{
		if(escaped[i] != '%')
		{
			uris.insert(uris.end(), escaped[i]);
			continue;
		}

		//invalid escape
		if(i + 2 >= escaped.size()) break;

		//% is always followed by 2 hexadecimal numbers
		char number[3] = {escaped[i + 1], escaped[i + 2], 0};
		auto num = std::strtol(number, nullptr, 16);

		//if we receive some invalid escape like "%yy" we will simply ignore it
		if(num) uris.insert(uris.end(), num);
		i += 2;
	}

	std::vector<std::string> ret;

	//split the list and check for comments if they should be removed
	//note that the uri spec sperates lines with "\r\n"
	while(true)
	{
		auto pos = uris.find("\r\n");
		if(pos == std::string::npos) break;

		auto uri = uris.substr(0, pos);
		if(!uri.empty() && ((uris[0] != '#') || !removeComments)) ret.push_back(std::move(uri));
		uris.erase(0, pos + 2);
	}

	return ret;
}

bool match(const DataFormat& dataFormat, nytl::StringParam formatName)
{
	if(sameBeginning(dataFormat.name, formatName.data())) return true;
	for(auto name : dataFormat.additionalNames)
		if(sameBeginning(name, formatName.data())) return true;
		
	return false;
}

//wrap
std::any wrap(std::vector<uint8_t> buffer, const DataFormat& fmt)
{
	if(fmt == DataFormat::text) return std::string(buffer.begin(), buffer.end());
	if(fmt == DataFormat::uriList) return decodeUriList({buffer.begin(), buffer.end()});
	if(fmt == DataFormat::imageData) return  deserializeImageData({buffer.data(), buffer.size()});

	return {std::move(buffer)};
}

std::vector<uint8_t> unwrap(std::any any, const DataFormat& format)
{
	if(format == DataFormat::text)
	{
		auto string = std::any_cast<const std::string&>(any);
		return {string.begin(), string.end()};
	}
	if(format == DataFormat::uriList)
	{
		auto uris = std::any_cast<const std::vector<std::string>&>(any);
		auto string = encodeUriList(uris);
		return {string.begin(), string.end()};
	}
	if(format == DataFormat::imageData)
	{
		auto id = std::any_cast<const OwnedImageData&>(any);
		return serialize({id.data.get(), id.size, id.format, id.stride});
	}

	return std::move(std::any_cast<std::vector<uint8_t>&>(any));
}

}
