// Boost Software License - Version 1.0 - August 17th, 2003
//
// Copyright (c) 2015-2016 nyorain
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#pragma once

#include <nytl/vec.hpp>
#include <memory>
#include <cstring>
#include <bitset>

//TODO: c++17 make ImageFormat pod struct derived from std::array and
//  the default formats constexpr members (i.e. ImageFormat::rgba8888)
//  make also the utility functions constexpr

namespace ny
{

///Represents a ColorChannel for an ImageFormat specification.
enum class ColorChannel
{
  none,
  red,
  green,
  blue,
  alpha
};

///An ImageFormat specifies the way the colors of an Image pixel is interpreted.
///It specifies the different used color channels and their size in bits.
///For offsets and spacings ColorChannel::none can be used. All other ColorChannels are allowed
///to appear only once per format (e.g. there can't be 8 red bits then 8 green bits and
///then 8 red bits again for one pixel).
///Note that the size of one channel must not be larger than 64 bits since larger values
///cannot be represented. Channels with a size of 0 should be ignored.
///There is place for 9 channels because a format can have 4 "real" channels (r,g,b,a) and
///then spacings (ColorChannel::none) between each of them in the beginning and end.
using ImageFormat = std::array<std::pair<ColorChannel, uint8_t>, 9>;

// - Default formats -
namespace imageFormats
{

constexpr ImageFormat rgba8888 {{
  {ColorChannel::red, 8},
  {ColorChannel::green, 8},
  {ColorChannel::blue, 8},
  {ColorChannel::alpha, 8},
}};

constexpr ImageFormat abgr8888 {{
  {ColorChannel::alpha, 8},
  {ColorChannel::blue, 8},
  {ColorChannel::green, 8},
  {ColorChannel::red, 8},
}};

constexpr ImageFormat argb8888 {{
  {ColorChannel::alpha, 8},
  {ColorChannel::red, 8},
  {ColorChannel::green, 8},
  {ColorChannel::blue, 8},
}};

constexpr ImageFormat bgra8888 {{
  {ColorChannel::blue, 8},
  {ColorChannel::green, 8},
  {ColorChannel::red, 8},
  {ColorChannel::alpha, 8},
}};

constexpr ImageFormat bgr888 {{
  {ColorChannel::blue, 8},
  {ColorChannel::green, 8},
  {ColorChannel::red, 8},
}};

constexpr ImageFormat rgb888 {{
  {ColorChannel::red, 8},
  {ColorChannel::green, 8},
  {ColorChannel::blue, 8},
}};

constexpr ImageFormat xrgb888 {{
  {ColorChannel::none, 8},
  {ColorChannel::red, 8},
  {ColorChannel::green, 8},
  {ColorChannel::blue, 8},
}};

constexpr ImageFormat a8 {{{ColorChannel::alpha, 8}}};
constexpr ImageFormat a1 {{{ColorChannel::alpha, 1}}};

constexpr ImageFormat r8 {{{ColorChannel::red, 8}}};
constexpr ImageFormat r1 {{{ColorChannel::red, 1}}};

constexpr ImageFormat g8 {{{ColorChannel::green, 8}}};
constexpr ImageFormat g1 {{{ColorChannel::green, 1}}};

constexpr ImageFormat b8 {{{ColorChannel::blue, 8}}};
constexpr ImageFormat b1 {{{ColorChannel::blue, 1}}};

}

///Returns whether the current machine is little endian.
///If this returns false it is assumed to be big endian (other types are ignored).
constexpr bool littleEndian();

///Returns the next multiple of alignment that is greater or equal than value.
///Can be used to 'align' a value e.g. align(27, 8) returns 32.
template<typename A, typename B>
constexpr auto align(A value, B alignment)
  { return alignment ? std::ceil(value / double(alignment)) * alignment : value; }

///Returns the number of bits needed to store one pixel in the given format.
unsigned int bitSize(const ImageFormat& format);

///Returns the number of bytes needed to store one pixel in the given format.
///Sine the exact value might not be a multiple of one byte, this value is rouneded up.
///Example: byteSize(ImageFormat::)
unsigned int byteSize(const ImageFormat& format);

///Converts between byte order (i.e. endian-independent) and word-order (endian-dependent)
///and vice versa. On a big-endian machine this function will simply return the given format, but
///on a little-endian machine it will reverse the order of the color channels..
///Note that BasicImage objects always hold data in word order, so if one works with a library
///that uses byte-order (as some e.g. image libraries do) they have to either convert the
///data or the format when passing/receiving from/to this library.
ImageFormat toggleByteWordOrder(const ImageFormat& format);

namespace detail
{
  template<typename T, typename F>
  void copy(T& to, F from, unsigned int) { to = from; }

  template<typename T, typename PF>
  void copy(T& to, const std::unique_ptr<PF[]>& from, unsigned int) { to = from.get(); }

  template<typename PT>
  void copy(std::unique_ptr<PT[]>& to, const uint8_t* from, unsigned int size)
  {
    if(!from)
    {
      to = {};
      return;
    }

    to = std::make_unique<PT[]>(size);
    std::memcpy(to.get(), from, size);
  }

  template<typename PT, typename PF>
  void copy(std::unique_ptr<PT>& to, const std::unique_ptr<PF[]>& from, unsigned int size)
  {
    if(!from)
    {
      to = {};
      return;
    }

    to = std::make_unique<PT[]>(size);
    std::memcpy(to.get(), from.get(), size);
  }
}

template<typename P> class BasicImage;
template<typename P> const uint8_t* data(const BasicImage<P>& img);
template<typename P> const uint8_t* data(const BasicImage<std::unique_ptr<P>>& img);
template<typename P> unsigned int dataSize(const BasicImage<P>& img);
template<typename P> unsigned int bitStride(const BasicImage<P>& img);

///Represents the raw data of an image as well infomrmation to interpret it.
///Note that this class does explicitly not implement any functions for creating/loading/changing
///the image itself, it is only used to hold all information needed to correctly interpret
///a raw image data buffer, generic functions for it can be created freely.
///Its stride is stored in bits and therefore the image stride might not be a multiple
///of 8 bits (1 byte).
///The format it holds is always specified in word order, that means if an image with
///a pixel 0xAABBCCDD has format rgba8888, this pixel will be interpreted as
///rgba(0xAA, 0xBB, 0xCC, 0xDD) independent from endianess (note that on little endian
///this is not how it is layed out in memory).
///There are several helper functions that make dealing with BasicImage objects easier.
///\tparam P The pointer type to used. Should be a type that can be used as std::uint8_t*.
///Might be a cv-qualified or smart pointer.
template<typename P>
class BasicImage
{
public:
  constexpr BasicImage() = default;
  ~BasicImage() = default;

  BasicImage(P xdata, nytl::Vec2ui xsize, const ImageFormat& fmt, unsigned int strd = 0u)
    : data(std::move(xdata)), size(xsize), format(fmt), stride(strd)
    { if(!stride) stride = size.x * bitSize(format); }

  template<typename O>
  BasicImage(const BasicImage<O>& lhs)
    : size(lhs.size), format(lhs.format), stride(bitStride(lhs))
    { detail::copy(data, lhs.data, dataSize(lhs)); }

  template<typename O>
  BasicImage& operator=(const BasicImage<O>& lhs)
  {
    size = lhs.size;
    format = lhs.format;
    stride = lhs.sitrde;
    detail::copy(data, lhs.data, dataSize(lhs));
    return *this;
  }

  constexpr BasicImage(BasicImage&&) = default;
  constexpr BasicImage& operator=(BasicImage&&) = default;

public:
  P data {}; //raw image data. References at least stride * size.y bits
  nytl::Vec2ui size {}; //size in pixels.
  ImageFormat format {}; //data format in word order (endian-native).
  unsigned int stride {}; //stride in bits. At least size.x * bitSize(format)
};

using Image = BasicImage<const uint8_t*>; ///Default, immutable, non-owned BasicImgae typedef.
using MutableImage = BasicImage<uint8_t*>; ///Mutable, non-owned BasicImage typedef
using UniqueImage = BasicImage<std::unique_ptr<uint8_t[]>>; ///Mutable, owned BasicImage typedef
using SharedImage = BasicImage<std::shared_ptr<uint8_t[]>>; ///Mutable, shared BasicImage typedef

///Returns the raw data from the given BasicImage as uint8_t pointer.
///Useful for generic code since it allows to access the raw data independently from the medium
///used to store it (could e.g. be a std::unique_ptr).
template<typename P>
constexpr uint8_t* data(const BasicImage<P>& img)
  { return img.data; }

template<typename P>
constexpr uint8_t* data(const BasicImage<std::unique_ptr<P>>& img)
  { return img.data.get(); }

///Returns the stride of the given BasicImage in bits.
///If the given image has no stride stored, calculates the stride.
template<typename P>
unsigned int bitStride(const BasicImage<P>& img)
  { return img.stride ? img.stride : img.size.x * bitSize(img.format); }

///Returns the stride of the given BasicImage in bytes (rounded up).
///If the given image has no stride stored, calculates the stride.
template<typename P>
unsigned int byteStride(const BasicImage<P>& img)
  { return img.stride ? std::ceil(img.stride / 8) : img.size.x * byteSize(img.format); }

///Returns the total amount of bytes the image data holds (rounded up).
template<typename P>
unsigned int dataSize(const BasicImage<P>& img)
  { return std::ceil(bitStride(img) * img.size.y / 8.0); }

///Returns the bit of the given Image at which the pixel for the given position begins.
unsigned int pixelBit(const Image&, nytl::Vec2ui position);

///Returns the color of the image at at the given position.
///Does not perform any range checking, i.e. if position lies outside of the size
///of the passed ImageData object, this call will result in undefined behavior.
nytl::Vec4u64 readPixel(const Image&, nytl::Vec2ui position);

///Returns the color of the given pixel value for the given ImageFormat.
///Results in undefined behaviour if the given format is invalid or the data referenced
///by pixel does not have enough bytes to read.
///\param bitOffset The bit position at which reading should start in signifance.
///E.g. if bitOffset is 5, we should start reading the 5th most significant bit,
///and then getting more significant by continuing with the 6th.
nytl::Vec4u64 readPixel(const uint8_t& pixel, const ImageFormat&, unsigned int bitOffset = 0u);

///Sets the color of the pixel at the given position.
///Does not perform any range checking, i.e. if position lies outside of the size
///of the passed ImageData object, this call will result in undefined behavior.
void writePixel(const MutableImage&, nytl::Vec2ui position, nytl::Vec4u64 color);

///Sets the color of the given pixel.
///Results in undefined behaviour if the given format is invalid or the data referenced
///by pixel does not have enough bytes to read.
///\param bitOffset The bit position at which reading should start in significance.
void writePixel(uint8_t& pixel, const ImageFormat&, nytl::Vec4u64 color,
  unsigned int bitOffset = 0u);

///Normalizes the given color values for the given format (color channel sizes).
///Example: norm({255, 128, 511, 0}, rgba8888) returns {1.0, 0.5, 2.0, 0.0}
nytl::Vec4f norm(nytl::Vec4u64 color, const ImageFormat& format);

///Makes sure that all color values can be represented by the number of bits their
///channel has in the given format while keeping the color as original as possible.
nytl::Vec4u64 downscale(nytl::Vec4u64 color, const ImageFormat& format);

//Returns whether an ImageData object satisfied the given requirements.
//Returns false if the stride of the given ImageData satisfies the given align but
//is not as small as possible.
///\param strideAlign The required alignment of the stride in bits
bool satisfiesRequirements(const Image&, const ImageFormat&, unsigned int strideAlign = 0);

///Can be used to convert image data to another format or to change its stride alignment.
///\param alignNewStride Can be used to pass a alignment requirement for the stride of the
///new (converted) data. Defaulted to 0, in which case the packed size will be used as stride.
///\sa BasicImageData
///\sa ImageDataFormat
UniqueImage convertFormat(const Image&, ImageFormat to, unsigned int alignNewStride = 0);
void convertFormat(const Image&, ImageFormat to, uint8_t& into, unsigned int alignNewStride = 0);

///Premutliplies the alpha factors for the given image.
void premultiply(const MutableImage& img);

/// - implementation -
constexpr bool littleEndian()
{
  constexpr uint32_t dummy = 1u;
  return (((uint8_t*)&dummy)[0] == 1);
}

unsigned int bitSize(const ImageFormat& format)
{
  auto ret = 0u;
  for(auto& channel : format) ret += channel.second;
  return ret;
}

unsigned int byteSize(const ImageFormat& format)
{
  return std::ceil(bitSize(format) / 8.0);
}

ImageFormat toggleByteWordOrder(const ImageFormat& format)
{
  if(!littleEndian()) return format;

  auto copy = format;
  auto begin = copy.begin();
  auto end = copy.end();

  //ignore "empty" channels (channels with size of 0)
  //otherwise toggles formats would maybe begin with x empty channels
  //which would be valid but really ugly
  while((begin != end) && (!begin->second)) ++begin;
  while((end != (begin + 1) && (!(end - 1)->second))) --end;

  std::reverse(begin, end);
  return copy;
}

unsigned int pixelBit(const Image& image, nytl::Vec2ui pos)
{
  return image.stride * pos.y + bitSize(image.format) * pos.x;
}

//TODO: find an actual big endian machine to test this on
nytl::Vec4u64 readPixel(const uint8_t& pixel, const ImageFormat& format, unsigned int bitOffset)
{
  const uint8_t* iter = &pixel;
  nytl::Vec4u64 rgba {};

  for(auto i = 0u; i < format.size(); ++i)
  {
    //for little endian channel order is inversed
    auto channel = (littleEndian()) ? format[format.size() - (i + 1)] : format[i];
    if(!channel.second) continue;

    //calculate the byte count we have to load at all for this channel
    unsigned int byteCount = std::ceil(channel.second / 8.0);

    uint64_t* val {};
    switch(channel.first)
    {
      case ColorChannel::red: val = &rgba[0]; break;
      case ColorChannel::green: val = &rgba[1]; break;
      case ColorChannel::blue: val = &rgba[2]; break;
      case ColorChannel::alpha: val = &rgba[3]; break;
      case ColorChannel::none: iter += byteCount; continue; //TODO: handle bitOffset
    }

    //reset the color value
    *val = {};

    //the bitset should store least significant bits/bytes (with data) first
    std::bitset<64> bitset {};

    //we simply iterate over all bytes/bits and copy them into the bitset
    //we have to respect byte endianess here
    if(littleEndian())
    {
      //for little endian we can simply copy the bits into the bitset bit by bit
      //the first bytes are the least significant ones, exactly as in the bitset
      for(auto i = 0u; i < channel.second; ++i)
      {
        //note that this does NOT extract the bit as position bitOffset but rather
        //the bitOffset-significant bit, i.e. we don't have to care about
        //bit-endianess in any way. We want less significant bits first and this is
        //what we get here (i.e. bitOffset will always only grow, therefore the
        //extracted bits will get more significant)
        bitset[i] = (*iter & (1 << bitOffset));

        ++bitOffset;
        if(bitOffset >= 8)
        {
          ++iter;
          bitOffset = 0;
        }
      }
    }
    else
    {
      //for big endian we have to swap the order in which we read bytes
      //we start at the most significant byte we have data for (since 0xFF should
      //result in 0xFF and not 0x000...00FF) and from there go backwards, i.e.
      //less significant byte-wise.
      //Bit-wise we still get more significant during each byte inside the loop.
      //The extra check (i == chanell.second) for the next byte is needed because
      //we only want to write the first (8 - bitOffset (from beginnig)) bits of
      //the first byte.
      //
      //Example for channel.second=14, bitOffset=3.
      //the resulting bitset and the iteration i that set the bitset value:
      //<6 7 8 9 10 11 12 13 | 0 1 2 3 4 5 - - | (here are 48 untouched bits)>
      //note how i=5 is the most significant bit for the color channel here.

      auto bit = channel.second - (channel.second % 8);
      for(auto i = 0; i < channel.second; ++i)
      {
        // const auto bit = channel.second - (currentByte * 8) + (8 - bitOffset);
        bitset[bit] = (*iter & (1 << bitOffset));

        ++bitOffset;
        ++bit;
        if(bitOffset >= 8 || i == (channel.second % 8) - 1)
        {
          ++iter;
          bit -= 8;
          bitOffset = 0;
        }
      }
    }

    //to_ullong returns an unsigned long long that has the first bits from the
    //bitset as least significant bits and the last bits from the bitset as most
    //significant bits
    *val = bitset.to_ullong();
  }

  return rgba;
}

void writePixel(uint8_t& pixel, const ImageFormat& format, nytl::Vec4u64 color,
  unsigned int bitOffset)
{
  uint8_t* iter = &pixel;

  for(auto i = 0u; i < format.size(); ++i)
  {
    //for little endian channel order is inversed
    auto channel = (littleEndian()) ? format[format.size() - (i + 1)] : format[i];
    if(!channel.second) continue;

    //calculate the byte count we have to load at all for this channel
    unsigned int byteCount = std::ceil(channel.second / 8.0);
    std::bitset<64> bitset;

    switch(channel.first)
    {
      case ColorChannel::red: bitset = color[0]; break;
      case ColorChannel::green: bitset = color[1]; break;
      case ColorChannel::blue: bitset = color[2]; break;
      case ColorChannel::alpha: bitset = color[3]; break;
      case ColorChannel::none: iter += byteCount; continue; //TODO: handle bitOffset
    }

    //this is exactly like readPixel but in the opposite direction
    if(littleEndian())
    {
      for(auto i = 0u; i < channel.second; ++i)
      {
        if(bitset[i]) *iter |= (1 << bitOffset);
        else *iter &= ~(1 << bitOffset);

        ++bitOffset;
        if(bitOffset >= 8u)
        {
          bitOffset = 0u;
          ++iter;
        }
      }
    }
    else
    {
      auto bit = channel.second - (channel.second % 8);
      for(auto i = 0; i < channel.second; i++)
      {
        if(bitset[channel.second - i]) *iter |= (1 << bitOffset);
        else *iter &= ~(1 << bitOffset);

        ++bitOffset;
        ++bit;
        if(bitOffset >= 8u || i == (channel.second % 8) - 1)
        {
          bitOffset = 0u;
          bit -= 8;
          ++iter;
        }
      }
    }
  }
}

nytl::Vec4u64 readPixel(const Image& img, nytl::Vec2ui pos)
{
  auto bit = pixelBit(img, pos);
  return readPixel(*(img.data + bit / 8), img.format, bit % 8);
}

void writePixel(const MutableImage& img, nytl::Vec2ui pos, nytl::Vec4u64 color)
{
  auto bit = pixelBit(img, pos);
  return writePixel(*(img.data + bit / 8), img.format, color, bit % 8);
}

nytl::Vec4f norm(nytl::Vec4u64 color, const ImageFormat& format)
{
  nytl::Vec4f ret = color;

  for(auto& channel : format)
  {
    if(!channel.second) continue;

    switch(channel.first)
    {
      case ColorChannel::red: ret[0] /= std::exp2(channel.second) - 1; break;
      case ColorChannel::green: ret[1] /= std::exp2(channel.second) - 1; break;
      case ColorChannel::blue: ret[2] /= std::exp2(channel.second) - 1; break;
      case ColorChannel::alpha: ret[3] /= std::exp2(channel.second) - 1; break;
      case ColorChannel::none: continue;
    }
  }

  return ret;
}

nytl::Vec4u64 downscale(nytl::Vec4u64 color, const ImageFormat& format)
{
  //find the smallest factor, i.e. the one we have to divide with
  auto factor = 1.0;
  for(auto channel : format)
  {
    auto value = 0u;
    switch(channel.first)
    {
      case ColorChannel::red: value = color[0]; break;
      case ColorChannel::green: value = color[1]; break;
      case ColorChannel::blue: value = color[2]; break;
      case ColorChannel::alpha: value = color[3]; break;
      case ColorChannel::none: continue;
    }

    if(!value) continue;
    auto highest = std::exp2(channel.second) - 1;
    factor = std::min(highest / value, factor);
  }

  return color * factor;
}

bool satisfiesRequirements(const Image& img, const ImageFormat& format,
  unsigned int strideAlign)
{
  auto smallestStride = img.size.x * bitSize(format);
  if(strideAlign) smallestStride = align(smallestStride, strideAlign);
  return (img.format == format && bitStride(img) == smallestStride);
}

UniqueImage convertFormat(const Image& img, ImageFormat to, unsigned int alignNewStride)
{
  auto newStride = img.size.x * bitSize(to);
  if(alignNewStride) newStride = align(newStride, alignNewStride);

  UniqueImage ret;
  ret.data = std::make_unique<std::uint8_t[]>(std::ceil((newStride * img.size.y) / 8.0));
  ret.size = img.size;
  ret.format = to;
  ret.stride = newStride;
  convertFormat(img, to, *ret.data.get(), alignNewStride);

  return ret;
}

void convertFormat(const Image& img, ImageFormat to, uint8_t& into, unsigned int alignNewStride)
{
  if(satisfiesRequirements(img, to, alignNewStride))
  {
    std::memcpy(&into, img.data, dataSize(img));
    return;
  }

  auto newStride = img.size.x * bitSize(to);
  if(alignNewStride) newStride = align(newStride, alignNewStride);

  for(auto y = 0u; y < img.size.y; ++y)
  {
    for(auto x = 0u; x < img.size.x; ++x)
    {
      auto color = downscale(readPixel(img, {x, y}), to);
      auto bit = y * newStride + x * bitSize(to);
      writePixel(*(&into + bit / 8), to, color, bit % 8);
    }
  }
}

void premultiply(const MutableImage& img)
{
  for(auto y = 0u; y < img.size.y; ++y)
  {
    for(auto x = 0u; x < img.size.x; ++x)
    {
      auto color = readPixel(img, {x, y});
      auto alpha = norm(color, img.format).w;
      color[0] *= alpha;
      color[1] *= alpha;
      color[2] *= alpha;
      writePixel(img, {x, y}, color);
    }
  }
}

}
