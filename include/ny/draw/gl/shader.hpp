#pragma once

#include <ny/include.hpp>
#include <ny/draw/gl/resource.hpp>

#include <nytl/vec.hpp>
#include <nytl/mat.hpp>

namespace ny
{

class GlTexture;

///The Shader class represents an OpenGL(ES) shader object.
///It can be created and compiled with an external source file or with a source string.
///One can use the uniform() function to set the shaders uniform parameters.
class Shader : public GlResource
{
public:
	//TODO: add more type options
	enum class Type
	{
		fragment,
		vertex
	};

protected:
    bool compile(const std::string& vertexShader, const std::string& fragmentShader);
    unsigned int program_ {0};

public:
    Shader();
    ~Shader();

    bool loadFromFile(const std::string& vertexFile, const std::string& fragmentFile);
    bool loadFromFile(const std::string& file, Type type);

    bool loadFromString(const std::string& vertexShader, const std::string& fragmentShader);
    bool loadFromString(const std::string& shader, Type type);

    void uniform(const std::string& name, float value);
    void uniform(const std::string& name, float x, float y);
    void uniform(const std::string& name, float x, float y, float z);
    void uniform(const std::string& name, float x, float y, float z, float w);

    void uniform(const std::string& name, const Vec2f& value);
    void uniform(const std::string& name, const Vec3f& value);
    void uniform(const std::string& name, const Vec4f& value);

    void uniform(const std::string& name, const Mat2f& value);
    void uniform(const std::string& name, const Mat3f& value);
    void uniform(const std::string& name, const Mat4f& value);

	void uniform(const std::string& name, const GlTexture& texture);

	///Returns the OpenGL(ES) handle to the shader program.
    unsigned int glProgram() const { return program_; }

	///Makes the shader program the used one in the calling thread/current context.
    void use() const;

	///Deletes the compiled shader program if existent.
	void reset();

	virtual bool shareable() const override { return 1; }
};

}