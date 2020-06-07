#version 460 core

layout(location = 0) in vec3 vpos;
layout(location = 3) in vec2 uv;


uniform mat4 model_mat;
uniform int x_res;
uniform int y_res;

out vec2 uv_vec;

void main()
{
    gl_Position = model_mat*vec4(vpos, 1.0)/vec4(x_res, y_res, 1, 1) - vec4(1,1,0,0);
    uv_vec = uv;
}
