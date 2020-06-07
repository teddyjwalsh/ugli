#pragma once
#ifndef GRAPHICS_GENERAL_H_
#define GRAPHICS_GENERAL_H_

#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>

#include "gl_includes.h"
#include "camera.h"
#include "object.h"
#include "shader.h"
#include "texture.h"
#include "definitions.h"

namespace graphics
{

extern GLuint active_program;
extern GLFWwindow* window;
extern std::shared_ptr<Camera> active_cam;
extern std::shared_ptr<Texture> draw_target;
extern std::shared_ptr<Program> active_program_ptr;

void set_program(GLuint in_program);

void set_program(std::shared_ptr<Program> in_program);

void set_active_camera(std::shared_ptr<Camera> in_cam);

void set_camera_matrices(std::shared_ptr<Camera> in_camera);

void set_camera_matrices(const Camera* in_camera);

std::shared_ptr<Camera> get_active_camera();

void error_callback(int error, const char* description);

void start_loop();

void end_loop();

void terminate();

void init(int x_res = 1024, int y_res = 1024);

void draw_object(std::shared_ptr<Object> in_object);

void draw_object(std::shared_ptr<Object> in_object,
    std::shared_ptr<Camera> in_camera);

void draw_object(std::shared_ptr<Object> in_object,
    const Camera* in_camera);

void set_draw_target(std::shared_ptr<Texture2D> in_tex = nullptr);

bool window_should_close();

void get_window_size(int& x_size, int& y_size);

}  // namespace graphics

#endif  // GRAPHICS_GENERAL_H_
