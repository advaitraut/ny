// Huge parts of this file (and the implementation) are basically just evg/image to keep
// independence from evg.

#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>

#include <memory>
#include <vector>

namespace ny
{

///The differents formats in which image data can be represented.
///For example, rgba8888 describes the following layout for std::uint8_t* data:
/// - data[0] is the r value
/// - data[1] is the g value
/// - data[2] is the b value
/// - data[3] is the a value
///\sa imageDataFormatSize
///\sa ImageData
enum class ImageDataFormat : unsigned int
{
	none,
	rgba8888,
	bgra8888,
	argb8888,
	rgb888,
	bgr888,
	a8
};

///Used to pass loaded or created images to functions.
///Note that this class does explicitly not implement any functions for creating/loading/changing
///the image itself, it is only used to hold all information needed to correctly interpret
///a raw image data buffer.
///\sa ImageDataFormat
///\sa imageDataFormatSize
struct ImageData
{
public:
	using Format = ImageDataFormat;

public:
	///The raw data of the image. Must be a pointer to at least stride * size.y bytes.
	const std::uint8_t* data {};

	///The size of the represented image.
	nytl::Vec2ui size {};

	///The format of the raw image data.
	ImageDataFormat format {};

	///The size in bytes of one image data row.
	///Note that this might be different from the packed size (size.x * imageDataFormatSize(format))
	///due to row alignment requirements.
	///If the stride is 0, it should always be treated as the packed size.
	unsigned int stride {};
};

///Represents an animated image, i.e. a collection of imageDatas with a stored delay between each
///other. Usually all stored images in the collection should have the same format, size and stride.
///Note that since it only holds ImageData objects, it does not own any of the image data buffers
///it holds.
///This class is intended as way to pass animated images (i.e. for cursors or icons) around that
///were loaded/created/manipulated using another toolkit.
///\sa ImageData
class AnimatedImageData
{
public:
	AnimatedImageData() = default;
	AnimatedImageData(const ImageData& image, unsigned int delay = 0) : images{{image, delay}} {}

	~AnimatedImageData() = default;
	AnimatedImageData(const AnimatedImageData& other) noexcept = default;
	AnimatedImageData& operator=(const AnimatedImageData& other) noexcept = default;

public:
	///A vector holding the collection of imageDatas as well as the delay in milliseconds
	///until the next image should be displayed.
	std::vector<std::pair<ImageData, unsigned int>> images;
};

///Returns the size of the given format in bytes.
///E.g. Format::rgba8888 would return 4, since one pixel of this format needs 4 bytes to
///be stored.
///\sa ImageDataFormat
unsigned int imageDataFormatSize(ImageDataFormat f);

///\{
///Can be used to convert image data to another format.
///\param stride The stride of the given image data. Defaulted to 0, in which case
///the given size * the size of the given format will be used as stride.
///The returned data will be tightly packed (no row paddings).
///\sa ImageData
///\sa ImageDataFormat
std::unique_ptr<std::uint8_t[]> convertFormat(const ImageData& img, ImageDataFormat to,
	unsigned int alignNewStride = 0);

void convertFormat(const ImageData& img, ImageDataFormat to, std::uint8_t& toData,
	unsigned int alignNewStride = 0);
///\}

}
