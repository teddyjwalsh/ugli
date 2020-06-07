#version 460 core

out vec4 frag_color;
layout(binding = 0, rgba32f) uniform image2D texinp;

in vec2 uv_vec;

void main()
{
    vec4 tex = imageLoad(texinp, ivec2(uv_vec));
    frag_color = vec4(tex.r);
    if (tex.r < 0.8)
    {
        discard;
    }
}
