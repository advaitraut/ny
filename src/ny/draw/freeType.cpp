#include <ny/draw/freeType.hpp>
#include <ny/draw/font.hpp>
#include <ny/base/log.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace ny
{

//FreeTypeLibrary
FreeTypeLibrary& FreeTypeLibrary::instance()
{
	static FreeTypeLibrary instance_;
	return instance_;
}

FreeTypeLibrary::FreeTypeLibrary()
{
    int error = FT_Init_FreeType(&lib_);
    if(error)
	{
		//throw
	}
}

FreeTypeLibrary::~FreeTypeLibrary()
{
	FT_Done_FreeType(lib_);
}

//FreeTypeFontHandle
FreeTypeFontHandle::FreeTypeFontHandle(const Font& font)
	: FreeTypeFontHandle(font.name(), font.fromFile())
{
}

FreeTypeFontHandle::FreeTypeFontHandle(const std::string& name, bool fromFile)
{
    std::string str = name;
    if(!fromFile)
    {
        str = "/usr/share/fonts/TTF/" + name;
		if(name.find('.') == std::string::npos)
		{
			str.append(".ttf");
		}
    }

    int ftErr = FT_New_Face(FreeTypeLibrary::instance().handle(), str.c_str(), 0, &face_);
    if(ftErr)
    {
        throw std::runtime_error("FTFont::FTFont: could lot load freetype font");
        return;
    }
}

FreeTypeFontHandle::~FreeTypeFontHandle()
{
    if(face_) FT_Done_Face(face_);
}

void FreeTypeFontHandle::characterSize(const Vec2ui& size)
{
	FT_Set_Pixel_Sizes(face_, size.x, size.y);
}

void FreeTypeFontHandle::cacheAscii() const
{
	for(unsigned char c = 0; c < 128; ++c)
	{
		auto it = charCache_.find(c);
		if(it != charCache_.cend() && it->second.image.data() != nullptr)
		{
			continue; //already loaded
		}

		if(FT_Load_Char(face_, c, FT_LOAD_RENDER))
		{
			continue; //ft failed, TODO warning/error
		}

		Character ret;
		ret.bearing = {face_->glyph->bitmap_left, face_->glyph->bitmap_top};
		ret.advance = face_->glyph->advance.x;

		auto size = Vec2ui{face_->glyph->bitmap.width, face_->glyph->bitmap.rows};
		ret.image = Image(face_->glyph->bitmap.buffer, size, Image::Format::a8);

		charCache_[c] = ret;
	}
}

Character& FreeTypeFontHandle::load(char c) const
{
	auto it = charCache_.find(c);
	if(it == charCache_.cend() || it->second.image.data() == nullptr)
	{
		auto ftErr = FT_Load_Char(face_, c, FT_LOAD_RENDER);
		if(ftErr)
		{
			throw std::runtime_error("Failed to load freeType char: " +
					std::to_string(static_cast<int>(c)) + ", Error: " + std::to_string(ftErr));
		}

		Character ret;
		ret.bearing = {face_->glyph->bitmap_left, face_->glyph->bitmap_top};
		ret.advance = face_->glyph->advance.x;

		auto size = Vec2ui{face_->glyph->bitmap.width, face_->glyph->bitmap.rows};
		ret.image = Image(face_->glyph->bitmap.buffer, size, Image::Format::a8);

		charCache_[c] = ret;
	}

	return charCache_[c];
}

}