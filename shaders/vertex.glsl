#version 460 core

layout(location = 0) in vec3 vpos;

uniform mat4 model_mat;

void main()
{
    gl_Position = model_mat*vec4(vpos, 1.0);
}