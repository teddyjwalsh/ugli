#pragma once
#ifndef GRAPHICS_TEXTURE_H_
#define GRAPHICS_TEXTURE_H_

#include "graphics/gl_includes.h"

namespace graphics
{

class Texture
{
public:
	Texture() :
		_texture_name(-1)
	{
		glGenTextures(1, &_texture_name);
	}

	GLuint get_texture_name() const
	{
		return _texture_name;
	}

private:
	GLuint _texture_name;
};

class Texture3D : public Texture
{
public:
	Texture3D(GLsizei size_x, GLsizei size_y, GLsizei size_z, GLenum format = GL_RGBA) :
		_size_x(size_x),
		_size_y(size_y),
		_size_z(size_z),
		_format(format)
	{
		bind();
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, _size_x, 0, format, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glGenerateMipmap(GL_TEXTURE_1D);
	}


	void set_data(int x_offset,
		int width,
		std::vector<glm::vec4> in_data)
	{
		glTextureSubImage1D(get_texture_name(),
			0,
			x_offset,
			width,
			_format,
			GL_FLOAT,
			in_data.data());
	}

	void get_data(std::vector<glm::vec4>& in_data)
	{
		in_data.resize(_size_x);
		glGetTextureImage(get_texture_name(),
			0,
			_format,
			GL_FLOAT,
			_size_x * sizeof(glm::vec4),
			in_data.data());
	}

	void bind()
	{
		glBindTexture(GL_TEXTURE_1D, get_texture_name());
	}

private:
	GLuint _size_x;
	GLuint _size_y;
	GLuint _size_z;

	GLenum _format;
};

class Texture1D : public Texture
{
public:
	Texture1D(GLsizei size, GLenum format = GL_RGBA) :
		_size(size),
		_format(format)
	{
		bind();
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, _size, 0, format, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glGenerateMipmap(GL_TEXTURE_1D);
	}


	void set_data(int x_offset,
		int width,
		std::vector<glm::vec4> in_data)
	{
		glTextureSubImage1D(get_texture_name(),
			0,
			x_offset,
			width,
			_format,
			GL_FLOAT,
			in_data.data());
	}

	void get_data(std::vector<glm::vec4>& in_data)
	{
		in_data.resize(_size);
		glGetTextureImage(get_texture_name(),
			0,
			_format,
			GL_FLOAT,
			_size * sizeof(glm::vec4),
			in_data.data());
	}

	void bind()
	{
		glBindTexture(GL_TEXTURE_1D, get_texture_name());
	}

private:
	GLuint _size;
	GLenum _format;
};

class Texture2D : public Texture
{
public:
	Texture2D(GLsizei size_x, GLsizei size_y, GLenum format = GL_RGBA) :
		_size_x(size_x),
		_size_y(size_y),
		_format(format)
	{
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _size_x, _size_y, 0, format, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);

		glGenTextures(1, &_depth_texture);
		glBindTexture(GL_TEXTURE_2D, _depth_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _size_x, _size_y, 0, format, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			std::cout << "FAIL " << err << "\n";
		}
	}

	void enable_rendering(bool enable)
	{
		if (!_rendering_enabled)
		{
			if (enable)
			{
				
				glGenFramebuffers(1, &_framebuffer_name);
				glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer_name);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, get_texture_name(), 0);
				//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
				_rendering_enabled = true;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}
		else
		{
			if (!enable)
			{
				glDeleteFramebuffers(1, &_framebuffer_name);
			}
		}
	}

	void draw_to_texture()
	{
		if (_rendering_enabled)
		{
			
			GLenum err;
			glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer_name);
			while ((err = glGetError()) != GL_NO_ERROR)
			{
				//std::cout << "NO0 " << err << "\n";
			}
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
			{
				//std::cout << "YES" << "\n";
			}
			while ((err = glGetError()) != GL_NO_ERROR)
			{
				//std::cout << "NO1 " << err << "\n";
			}
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			while ((err = glGetError()) != GL_NO_ERROR)
			{
				//std::cout << "NO " << err << "\n";
			}
		}
	}

	void set_data(int x_offset, int y_offset, int width, int height,
		std::vector<glm::vec4> in_data)
	{
		glTextureSubImage2D(get_texture_name(),
			0,
			x_offset,
			y_offset,
			width,
			height,
			_format,
			GL_FLOAT,
			in_data.data());
	}

	void get_data(std::vector<glm::vec4>& in_data)
	{
		in_data.resize(_size_x * _size_y);
		glGetTextureImage(get_texture_name(),
			0,
			_format,
			GL_FLOAT,
			_size_x * _size_y * sizeof(glm::vec4),
			in_data.data());
	}

	bool rendering_enabled() const
	{
		return _rendering_enabled;
	}

	GLenum get_format() const
	{
		return _format;
	}

	void bind()
	{
		glBindTexture(GL_TEXTURE_2D, get_texture_name());
	}

private:
	GLuint _depth_texture;
	GLuint _framebuffer_name;
	GLint _size_x;
	GLint _size_y;
	bool _rendering_enabled;
	GLenum _type;
	GLenum _format;
};

}  // namespace graphics

#endif GRAPHICS_TEXTURE_H_
