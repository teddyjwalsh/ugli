#pragma once
#ifndef GRAPHICS_FRAMEBUFFER_H_
#define GRAPHICS_FRAMEBUFFER_H_

#include <GLFW/glfw3.h>
#include <GL/GL.h>

#include "graphics/texture.h"

namespace graphics
{

class Framebuffer
{
public:
	Framebuffer() :
		_framebuffer_name(-1)
	{
		glGenFramebuffers(1, &_framebuffer_name);
	}

	void attach_texture(std::shared_ptr<Texture> in_tex)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer_name);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, in_tex->)
	}

private:
	GLuint _framebuffer_name;
};

}  // namespace graphics

#endif  // GRAPHICS_FRAMEBUFFER_H_