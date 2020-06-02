#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(binding = 0, rgba32f) uniform image1D cube_locs;
layout(binding = 1, rgba32f) uniform image2D out_tex;
layout(binding = 2, rgba32f) uniform image2D norm_tex;
layout(binding = 3, rgba32f) uniform image1D cube_colors;
// The camera specification
uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;
uniform int time;

#define MAX_SCENE_BOUNDS 1000.0

vec3 light_pos = vec3(5, 10, 10);

struct box {
    vec3 min;
    vec3 max;
};

float random(vec2 st) {
    return fract(sin(dot(st.xy,
        vec2(12.9898, 78.233))) *
        43758.5453123);
}

float random2(vec2 st) {
    return fract(sin(dot(st.xy,
        vec2(25.187, 145.23))) *
        43758.5453123);
}

float random3(vec2 st) {
    return fract(sin(dot(st.xy,
        vec2(54.789, 10.12))) *
        43758.5453123);
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
        oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
        oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

vec2 intersectBox(vec3 origin, vec3 dir, const vec3 box_origin) {
    vec3 b_min = box_origin;
    vec3 b_max = box_origin + vec3(1.0, 1.0, 1.0);
    vec3 tMin = (b_min - origin) / dir;
    vec3 tMax = (b_max - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    return vec2(tNear, tFar);
}

struct hitinfo {
    vec2 lambda;
    int bi;
    float mag;
    vec3 refl;
    vec3 hit_loc;
    vec3 box_loc;
    vec3 norm;
    vec3 color;
    float travel_dist;
};

vec3 reflect(vec3 d, vec3 n)
{
    return d - 2 * (dot(d, n)) * n;
}

bool intersectBoxes(vec3 origin, vec3 dir, out hitinfo info) {
    float smallest = MAX_SCENE_BOUNDS;
    bool found = false;
    int cube_tex_size = imageSize(cube_locs);
    info.travel_dist = 0;

    for (int i = 0; i < cube_tex_size; i++) 
    {
        vec4 box_origin = imageLoad(cube_locs, i);
        vec4 box_color = imageLoad(cube_colors, i);
        vec2 lambda = intersectBox(origin, dir, box_origin.xyz);
        
        if (lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) 
        {
            return true;
            info.lambda = lambda;
            info.bi = i;
            smallest = lambda.x;
            found = true;
            vec3 int_pos = origin + dir * smallest*0.999;

            float x_dist = abs(int_pos.x - round(int_pos.x));
            float y_dist = abs(int_pos.y - round(int_pos.y));
            float z_dist = abs(int_pos.z - round(int_pos.z));
            vec3 norm = vec3(0.0, 1.0, 0.0);
            if (x_dist < y_dist && x_dist < z_dist)
            {
                norm = vec3(1.0, 0.0, 0.0);
            }
            if (z_dist < y_dist && z_dist < x_dist)
            {
                norm = vec3(0.0, 0.0, 1.0);
            }
            norm = normalize(norm);
            if (dot(dir, norm) > 0)
            {
                norm = -norm;
            }
            vec3 reflect = dir - 2 * (dot(dir, norm)) * norm;
            vec3 light_dir = normalize(light_pos - int_pos);
            info.mag = max(0, dot(light_dir, norm));
            info.refl = reflect;
            info.hit_loc = int_pos;
            info.travel_dist = length(int_pos - origin);
            info.norm = norm;
            info.color = vec3(box_color);
            info.box_loc = box_origin.xyz;
        }
    }
    return found;
}

vec4 trace(vec3 origin, vec3 dir, out vec4 out_norm) {
    hitinfo i;
    hitinfo light_i;
    hitinfo i2;
    int count = 0;
    int f_count = 1;
    vec3 out_color = vec3(0.0, 0.0, 0.0);
    i.hit_loc = origin;
    i.refl = dir;
    float total_dist = 0;
    out_norm = vec4(0, 0, 0, 0);
    while (f_count < 2)
    {
        i.color = vec3(1.0);
        i.hit_loc = origin;
        i.refl = dir;
        i.norm = vec3(0, 0, 0);
        count = 0;
        total_dist = 0;
        while (count < 3)
        {
            bool hit = intersectBoxes(i.hit_loc, i.refl, i2);
            
            i2.color = i2.color * i.color;
            if (hit)
            {
                return vec4(1.0, 1.0, 1.0, 1.0);
                if (f_count == 1 && count == 0)
                {
                    out_norm = vec4(i2.norm, length(eye - i2.box_loc));
                }
                vec3 light_dir = normalize(light_pos - i2.hit_loc);
                bool light_hit = intersectBoxes(i2.hit_loc, light_dir, light_i);
                //out_color += dot(light_dir, i2.norm) * vec3(1.0, 1.0, 1.0);
                if (light_hit)
                {

                    //out_color += dot(light_dir, i.refl) * vec3(1.0, 1.0, 1.0);
                }
                else
                {
                    //out_color += vec3(0.5, 0.5, 0.5);
                    float light_dist = total_dist + length(light_pos - i2.hit_loc);
                    out_color += 100.0*max(0,dot(light_dir, i2.norm)) * i2.color / (light_dist * light_dist);
                }
            }
            else
            {
                break;
            }

            float draw  = random(mod(gl_GlobalInvocationID.xy*(f_count^1234), 1024) / vec2(1024, 1024));
            float draw2 = random2(mod(gl_GlobalInvocationID.xy*(f_count^4356), 1024) / vec2(1024, 1024));
            float draw3 = random3(mod(gl_GlobalInvocationID.xy*(f_count^7890), 1024) / vec2(1024, 1024));

            i2.refl = normalize(mix(i2.refl, i2.norm, 0.99) + 1.0*vec3(draw, draw2, draw3));
            if (count > 0)
            {
                total_dist += i2.travel_dist;
            }
            i = i2;

            count += 1;
        }
        
        f_count += 1;
    }
    return vec4(out_color, 1.0);
}

void main()
{
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(out_tex);
    vec4 frag_norm;
    
    vec3 init_vec = vec3(1.0, 0.0, 0.0);

    int count = 0;

    if (pix.x >= size.x || pix.y >= size.y) 
    {
        return;
    }
    vec2 pos = vec2(pix) / vec2(size.x, size.y);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);

    dir = normalize(dir);
    vec4 color = trace(eye, dir, frag_norm);
    //for (int i = 0; i < cube_tex_size.x; ++i)
    //{
     //   imageStore(cube_locs, i, vec4(pix.x * 1.0 / 100.0, pix.y * 1.0 / 100.0, 0.5, cube_tex_size * 1.0 / 100.0));
     //   count += 1;
    //}
    imageStore(norm_tex, pix, frag_norm);
    imageStore(out_tex, pix, color);
}