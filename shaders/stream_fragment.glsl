#version 460 core

out vec4 frag_color;

in vec3 normal;
in vec2 v_color;

const vec3 light_dir = vec3(1,-1,0);

void main()
{
    float raw_light = length(v_color);//dot(normal, light_dir)*;
    //float raw_light = abs(dot(normal, normalize(light_dir)));
    frag_color = vec4(vec3(1.0)*(log(20*raw_light + 1)/5.0),1.0);//;
}
