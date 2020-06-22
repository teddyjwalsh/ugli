#version 460 core

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 vnorm;
layout(location = 3) in vec2 vuv;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 projection_mat;

out vec3 normal;
out vec2 v_color;

void main()
{
    // note that we read the multiplication from right to left
    gl_Position = projection_mat * view_mat * model_mat * vec4(vpos, 1.0);
	normal = vnorm;
	v_color = vuv;
}
