#pragma once
#ifndef GRAPHICS_DEFINITIONS_H_
#define GRAPHICS_DEFINITIONS_H_

#include <string>
#include <memory>

#include "graphics/gl_includes.h"

namespace graphics
{

const std::string VIEW_MAT_NAME = "view_mat";
const std::string PROJECTION_MAT_NAME = "projection_mat";
extern GLuint active_program;
extern GLFWwindow* window;

class Program;

void set_program(GLuint in_program);

void set_program(std::shared_ptr<Program> in_program);

}

#endif  // GRAPHICS_DEFINITIONS_H_
