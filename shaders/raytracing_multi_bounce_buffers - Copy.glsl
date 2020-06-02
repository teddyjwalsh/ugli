#version 460 core

struct chunk_alloc
{
    vec3 coord;
    int mem_index;
};

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(std430, binding = 0) buffer layoutName1
{
    vec4 cube_locs[];
};
layout(std430, binding = 3) buffer layoutName2
{
    vec4 cube_colors[];
};
layout(std430, binding = 4) buffer layoutName3
{
    int cube_states[];
};
layout(std430, binding = 5) buffer layoutName4
{
    vec4 textures[];
};
layout(std430, binding = 6) buffer layoutName5
{
    chunk_alloc chunks[];
};
layout(binding = 1, rgba32f) uniform image2D out_tex;
layout(binding = 2, rgba32f) uniform image2D norm_tex;

// The camera specification
uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;
uniform int ttime;
uniform int cube_count_x;
uniform int cube_count_y;
uniform int cube_count_z;
uniform int tex_width;
uniform int tex_height;
uniform int chunk_count;
uniform int chunk_size_x;
uniform int chunk_size_y;
uniform int chunk_size_z;

#define MAX_SCENE_BOUNDS 1000.0

vec3 light_pos = vec3(5000, 10000, 10000);
int max_count = 5;
int chunk_block_count = chunk_size_x * chunk_size_y * chunk_size_z;

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

vec4 get_tex_val(float x, float y, int tex_index)
{
    int x_int = int(x * tex_width);
    int y_int = int(y * tex_height);
    return textures[x_int + tex_width * y_int + tex_width*tex_height*tex_index];
}

chunk_alloc get_chunk_index(vec3 in_pos)
{
    
    for (int i = 0; i < chunk_count; ++i)
    {
        //chunk_alloc default_alloc2;
        //default_alloc2.mem_index = 0;
        //return default_alloc2;
        if (chunks[i].mem_index > chunk_count)
        {
            break;
        }
        float dx = in_pos.x - chunks[i].coord.x;
        float dy = in_pos.y - chunks[i].coord.y;
        float dz = in_pos.z - chunks[i].coord.z;

        if (dz < chunk_size_z && dz >= 0 &&
            dy < chunk_size_y && dy >= 0 &&
            dx < chunk_size_x && dx >= 0)
        {
            return chunks[i];
        }
    }
    chunk_alloc default_alloc;
    default_alloc.mem_index = -1;
    return default_alloc;
}

int get_cube(vec3 in_pos, chunk_alloc in_chunk)
{
    //if (in_pos.y < -3)
    //{
   //     return 1;
   // }
    
    //in_chunk = get_chunk_index(in_pos);
    
    //if (in_chunk.mem_index >= 0)
    //if (in_pos.x < cube_count_x / 2 && in_pos.x > -cube_count_x / 2 &&
    //    in_pos.y < cube_count_y / 2 && in_pos.y > -cube_count_y / 2 &&
    //    in_pos.z < cube_count_z / 2 && in_pos.z > -cube_count_z / 2)
    {
        //if (in_pos.y < -2)
        //{
        //    return 1;
        //}
        //return 0;
                return cube_states[chunk_block_count*in_chunk.mem_index + 
                    int(floor(in_pos.x - in_chunk.coord.x)) +
                    chunk_size_x * int(floor(in_pos.y - in_chunk.coord.y)) +
                    chunk_size_y * chunk_size_x * int(floor(in_pos.z - in_chunk.coord.z))];
    }

    //return 0;
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

    float intersectBoxScale(vec3 origin, vec3 dir, const vec3 box_origin, vec3 scale) 
    {
        vec3 b_min = box_origin;
        vec3 b_max = box_origin + scale;
        vec3 tMin = (b_min - origin) / dir;
        vec3 tMax = (b_max - origin) / dir;
        vec3 t1 = min(tMin, tMax);
        vec3 t2 = max(tMin, tMax);
        float tNear = max(max(t1.x, t1.y), t1.z);
        float tFar = min(min(t2.x, t2.y), t2.z);
        return tFar;
    }

float intersectBox(vec3 origin, vec3 dir, const vec3 box_origin) {
    
    vec3 b_min = box_origin;
    vec3 b_max = box_origin + vec3(1.0);
    vec3 inv = 1.0 / dir;
    vec3 tMin = (b_min - origin) * inv;
    vec3 tMax = (b_max - origin) * inv;
    //vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    //vec3 t2 = step(tMin, tMax) * tMax + step(tMax, tMin) * tMin;
    //vec3 t2 = vec3(mix(tMin.x, tMax.x, step(tMin.x, tMax.x)), mix(tMin.y, tMax.y, step(tMin.y, tMax.y)), mix(tMin.z, tMax.z, step(tMin.z, tMax.z)));
    //float tNear = max(max(t1.x, t1.y), t1.z);
    
    float tFar = min(min(t2.x, t2.y), t2.z);

    return tFar;
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

int intersectBoxesFast(vec3 origin, vec3 dir, out hitinfo info)
{
    float smallest = MAX_SCENE_BOUNDS;
    int found = 0;
    int cube_tex_size = cube_count_x;// imageSize(cube_locs);
    info.travel_dist = 0;
    float leng = 0.0001;
    float short_length = 0.0001;
    float t = 0.00;
    float chunk_limit = 0;
    int prev_cube_type = 0;
    int cube_type = 0;
    vec3 chunk_scale = vec3(chunk_size_x, chunk_size_y, chunk_size_z);
    chunk_alloc cur_chunk_index = get_chunk_index(origin);

    for (int i = 0; i < 100; ++i)
    //while (leng < 100)
    {
        vec3 loc = origin + dir * leng;
        if (leng > chunk_limit)
        {
            //vec3 chunk_origin = vec3(chunk_size_x * floor(loc.x / chunk_size_x),
            //   chunk_size_y * floor(loc.y / chunk_size_y),
            //    chunk_size_z * floor(loc.z / chunk_size_z));
            cur_chunk_index = get_chunk_index(loc);
            chunk_limit = intersectBoxScale(origin, dir, cur_chunk_index.coord, chunk_scale);
            
        }
        vec3 cur_box = floor(loc);
        if (cur_chunk_index.mem_index >= 0)
        {
            prev_cube_type = cube_type;
            cube_type = get_cube(loc, cur_chunk_index);
        }
        else
        {
            leng = chunk_limit + 0.001;
            //continue;
        }



        if ((cube_type > 0 || (prev_cube_type == 2 && cube_type == 0)) && cube_type != prev_cube_type)
            //if(loc.x < 10 && loc.y < -2 && loc.z < 10)
        {
            vec3 int_pos = origin + dir * short_length;
            
            float x_dist = abs(int_pos.x - round(int_pos.x));
            float y_dist = abs(int_pos.y - round(int_pos.y));
            float z_dist = abs(int_pos.z - round(int_pos.z));
            float x_f = abs(int_pos.x - floor(int_pos.x));
            float y_f = abs(int_pos.y - floor(int_pos.y));
            float z_f = abs(int_pos.z - floor(int_pos.z));
            vec3 norm = vec3(0.0, 1.0, 0.0);
            vec3 rel_up = vec3(1.0, 0.0, 0.0);
            float x_tex = x_f;
            float y_tex = z_f;
            int tex_index = 1;
            if (x_dist < y_dist && x_dist < z_dist)
            {
                norm = vec3(1.0, 0.0, 0.0);
                rel_up = vec3(0.0, 1.0, 0.0);
                x_tex = z_f;
                y_tex = 1-y_f;
                tex_index = 0;
            }
            if (z_dist < y_dist && z_dist < x_dist)
            {
                norm = vec3(0.0, 0.0, 1.0);
                rel_up = vec3(0.0, 1.0, 0.0);
                x_tex = x_f;
                y_tex = 1-y_f;
                tex_index = 0;
            }
            
            if (dot(dir, norm) > 0)
            {
                norm = -norm;
            }
            
            info.color = get_tex_val(x_tex, y_tex, tex_index).xyz;
            //norm = normalize(norm + 0.1*(info.color.x-1.0)*(rel_up));

            if (cube_type == 2)
            {
                origin = loc;
                leng = 0.001;
                //float theta1 = acos(dot(norm, normalize(dir)));
                //float theta2 = asin(sin(theta1) * 1.33 / 1.0);
                float r = 1.0 / 1.33;
                vec3 n = norm;
                vec3 V_incedence = normalize(dir);
                float c = dot(V_incedence, -n);
                vec3 V_refraction = r * V_incedence + (r * c - sqrt(1 - pow(r, 2)*(1 - pow(c, 2)))) * n;
                dir = V_refraction;
            }
            else if (cube_type == 0 && prev_cube_type == 2)
            {
                origin = loc;
                leng = 0.001;
                //float theta1 = acos(dot(norm, normalize(dir)));
                //float theta2 = asin(sin(theta1) * 1.33 / 1.0);
                float r = 1.33 / 1.00;
                vec3 n = norm;
                vec3 V_incedence = normalize(dir);
                float c = dot(V_incedence, -n);
                vec3 V_refraction = r * V_incedence + (r * c - sqrt(1 - pow(r, 2) * (1 - pow(c, 2)))) * n;
                dir = V_refraction;
            }
            else
            {
                info.norm = norm;
                vec3 reflect = dir - 2 * (dot(dir, norm)) * norm;
                vec3 light_dir = normalize(light_pos - int_pos);
                info.mag = max(0, dot(light_dir, norm));
                info.refl = reflect;


                info.hit_loc = int_pos;
                info.travel_dist = short_length;
            }
            

            info.box_loc = cur_box;
            //return true;
            if (cube_type == 1)
            {
                found = 1;
                break;
            }
        }
        else if (cube_type < 0)
        {
            found = 0;
            break;
            //return false;
        }
        t = intersectBox(origin, dir, cur_box);
        if (t <= leng)
        {
            found = 0;
            break;
            //return false;
        }
        leng = t+0.0001;
        short_length = t-0.0001;

    }
    if (found == 0)
    {
        info.color = vec3(0.5, 0.5, 1.0)/max_count;
        return 0;
    }
    return found;
}
/*
bool intersectBoxes(vec3 origin, vec3 dir, out hitinfo info) {
    float smallest = MAX_SCENE_BOUNDS;
    bool found = false;
    int cube_tex_size = cube_count_x;// imageSize(cube_locs);
    info.travel_dist = 0;

    for (int i = 0; i < cube_tex_size; i++) 
    {
        vec4 box_origin = cube_locs[i];// imageLoad(cube_locs, i);
        vec4 box_color = cube_colors[i];// imageLoad(cube_colors, i);
        vec2 lambda = intersectBoxesFast(origin, dir, box_origin.xyz);
        
        if (lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) 
        {
            //return true;
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
*/

vec4 trace(vec3 origin, vec3 dir, out vec4 out_norm) {
    hitinfo i;
    hitinfo light_i;
    hitinfo i2;
    bool first_hit = false;
    int count = 0;
    int f_count = 1;
    vec3 out_color = vec3(0.0);
    i.hit_loc = origin;
    i.refl = dir;
    float total_dist = 0;
    out_norm = vec4(0);

    vec3 first_reflection_origin = origin;
    vec3 first_reflection_dir = dir;
    for (f_count = 1; f_count < max_count; ++f_count)
    //while (f_count < 3)
    {
        i.color = vec3(1.0);
        i.hit_loc = first_reflection_origin;
        i.refl = first_reflection_dir;
        i.norm = vec3(0);
        if (f_count > 0)
        {
            count = 1;
        }
        else
        {
            count = 0;
        }
        total_dist = 0;
        for (count; count < 8; ++count)
        //while (count < 4)
        {

            if (count > 1)
            {
                total_dist += i2.travel_dist;
            }

            int hit = intersectBoxesFast(i.hit_loc, i.refl, i2);

            if (hit > 0)
            {
                i2.color = i2.color * i.color;
                //return vec4(1.0, 1.0, 1.0, 1.0);
                if (f_count == 1 && count == 0)
                {
                    out_norm = vec4(i2.norm, length(eye - i2.box_loc));
                    first_reflection_origin = i2.hit_loc;
                    first_reflection_dir = i2.refl;
                }
                vec3 light_dir = normalize(light_pos - i2.hit_loc);
                int light_hit = intersectBoxesFast(i2.hit_loc, light_dir, light_i);
                //out_color += dot(light_dir, i2.norm) * vec3(1.0, 1.0, 1.0);
                if (light_hit > 0)
                {

                    //out_color += dot(light_dir, i.refl) * vec3(1.0, 1.0, 1.0);
                }
                else
                {
                    //out_color += vec3(0.5, 0.5, 0.5);
                    //if (count == 1)// || count == 0)
                    {
                        float light_dist = total_dist + length(light_pos - i2.hit_loc);
                        out_color += (50000000.0 * max(0, dot(light_dir, i2.norm)) * i2.color / (light_dist * light_dist));
                    }
                }
                first_hit = true;
            }
            else
            {
                if (count > 1)
                {
                    i2.color = i2.color * i.color;
                    out_color += 1 * max(0, dot(i.refl, i2.norm)) * i2.color;
                }
                else
                {
                    out_color = vec3(0.8, 0.8, 1.0);
                }
                break;
            }

            float draw = random(mod(gl_GlobalInvocationID.xy * (f_count ^ 1234), 1024) / vec2(1024, 1024));
            float draw2 = random2(mod(gl_GlobalInvocationID.xy * (f_count ^ 4356), 1024) / vec2(1024, 1024));
            float draw3 = random3(mod(gl_GlobalInvocationID.xy * (f_count ^ 7890), 1024) / vec2(1024, 1024));
            float mix_val = 0.9;
            i2.refl = normalize((i2.refl*(1-mix_val) + i2.norm*mix_val)/1.0 + 0.6 * vec3(draw, draw2, draw3));
            
            i = i2;

            //count += 1;
        }
        if (!first_hit)
        {
            break;
        }
        //f_count += 1;
    }
    return vec4(out_color, 1.0);
}

void main()
{
    light_pos = vec3(5000 * cos(2 * 3.14159 * (ttime%200)/200.0), 5000*sin(2*3.14159*(ttime%200)/200.0), 10000);
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