#version 460 core

layout(location = 0) in vec3 vpos;
layout(location = 3) in vec2 uv;


uniform mat4 model_mat;

out vec2 uv_vec;

void main()
{
    gl_Position = model_mat*vec4(vpos, 1.0);
    uv_vec = uv;
}
