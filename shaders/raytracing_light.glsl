#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(binding = 0, rgba32f) uniform image1D cube_locs;
layout(binding = 1, rgba32f) uniform image2D out_tex;
// The camera specification
uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

#define MAX_SCENE_BOUNDS 1000.0

vec3 light_dir = normalize(vec3(-0.5, -1.0, -0.3));

struct box {
    vec3 min;
    vec3 max;
};

float random(vec2 st) {
    return fract(sin(dot(st.xy,
        vec2(12.9898, 78.233))) *
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
};

vec3 reflect(vec3 d, vec3 n)
{
    return d - 2 * (dot(d, n)) * n;
}

bool intersectBoxes(vec3 origin, vec3 dir, out hitinfo info) {
    float smallest = MAX_SCENE_BOUNDS;
    bool found = false;
    int cube_tex_size = imageSize(cube_locs);
    for (int i = 0; i < cube_tex_size; i++) {
        vec4 box_origin = imageLoad(cube_locs, i);
        vec2 lambda = intersectBox(origin, dir, box_origin.xyz);
        if (lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) {
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
            info.mag = 0.7 * max(0, -dot(light_dir, norm)) + 0.1 * max(0, exp(-dot(light_dir, reflect)));
            info.refl = reflect;
            info.hit_loc = int_pos;
        }
    }
    return found;
}

vec4 trace(vec3 origin, vec3 dir) {
    hitinfo i;
    hitinfo i2;
    int count = 0;
    int f_count = 0;
    vec3 out_color = vec3(0.0, 0.0, 0.0);
    while (f_count < 4)
    {
        i.hit_loc = origin;
        i.refl = dir;
        while (count < 4)
        {
            if (intersectBoxes(i.hit_loc, i.refl, i)) {
                out_color += 0.2*vec3(1.0, 1.0, 1.0) * dot(i.refl, -light_dir);
            }
            else
            {
                out_color += 0.2*vec3(1.0, 1.0, 1.0) * i.mag;
            }
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
    
    int count = 0;

    if (pix.x >= size.x || pix.y >= size.y) 
    {
        return;
    }
    vec2 pos = vec2(pix) / vec2(size.x, size.y);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);
    dir = normalize(dir);
    vec4 color = trace(eye, dir);
    //for (int i = 0; i < cube_tex_size.x; ++i)
    //{
     //   imageStore(cube_locs, i, vec4(pix.x * 1.0 / 100.0, pix.y * 1.0 / 100.0, 0.5, cube_tex_size * 1.0 / 100.0));
     //   count += 1;
    //}

    imageStore(out_tex, pix, color);
}