#version 460 core

#define MAX_CHILD_RAYS 3
#define MAX_BOUNCES 5
#define MAX_INITIAL_SHOTS 1
#define ARRAY_SIZE 5
#define MAX_CHUNK_INFOS 10
#define MAX_OCTREE_ELEMENTS 100000

#define GROUP_SIZE_X 8
#define GROUP_SIZE_Y 8
#define GROUP_SIZE_Z 1

#define valtype vec4

layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;

struct Octree
{
    float size; // side size
    vec4 pos; // "floor" corner of bounds
    int children[8];
    valtype value;
    int parent;
    //int pad[3];
};

struct BoxObject
{
    vec4 loc;
    vec4 size;
    mat4 transform;
};

layout(std430, binding = 3) buffer layoutName6
{
    int chunk_map[];
};
layout(std430, binding = 4) buffer layoutName3
{
    int cube_states[];
};
layout(std430, binding = 5) buffer layoutName4
{
    vec4 textures[];
};
layout(std430, binding = 7) buffer layoutName5
{
    Octree g_oct_buf[];
};



layout(std430, binding = 20) buffer layoutName20
{
    vec4 mip_map1[];
};
layout(std430, binding = 21) buffer layoutName21
{
    vec4 mip_map2[];
};
layout(std430, binding = 22) buffer layoutName22
{
    vec4 mip_map3[];
};

layout(binding = 1, rgba32f) uniform image2D out_tex;
layout(binding = 3, rgba32f) uniform image2D out_tex_bounce_pass;
layout(binding = 7, rgba32f) uniform image2D out_tex_low_res;
layout(binding = 6, rgba32f) uniform image2D norm_tex;
layout(binding = 2, rgba32f) uniform image2D norm_tex_low_res;
layout(binding = 0, rgba32f) uniform image2D pos_tex;

struct HitInfo
{
    int ray_id;
    vec3 position;
    vec3 normal;
    vec4 reflect_color;
    vec4 refract_color;
    vec3 incident;
    float refraction_index;
    float distance;
    bool hit;
    int block_type;
    vec2 tex_coords;
    int side_index;
    vec3 cube_center;
};

struct Ray
{
    vec3 origin;
    vec3 dir;
    vec4 color;
    ivec2 pixel;
    int bounces;
    //int children[MAX_CHILD_RAYS];
};

// Globals
    // The camera specification
uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;
uniform double ttime;
uniform int cube_count_x;
uniform int cube_count_y;
uniform int cube_count_z;
uniform int tex_width;
uniform int tex_height;
uniform int tex_count;
uniform int chunk_count;
uniform int chunk_size_x;
uniform int chunk_size_y;
uniform int chunk_size_z;
uniform int chunk_map_size_x;
uniform int chunk_map_size_y;
uniform int chunk_map_size_z;
uniform int max_bounces;
uniform int include_first_bounce;

struct Lamp
{
    vec4 loc;
    vec4 color;
    float intensity;
};

struct ChunkInfo
{
    vec4 coord;
    int index;
};

struct ChunkTraversal
{
    ChunkInfo info;
    vec2 limits;
};

struct ChunkTraversals
{
    int count;
    ChunkTraversal infos[MAX_CHUNK_INFOS];
};

int chunk_block_count = chunk_size_x * chunk_size_y * chunk_size_z;
vec3 chunk_scale = vec3(chunk_size_x, chunk_size_y, chunk_size_z);
const int num_lamps = 1;
const int num_initial_shots_from_pixel = 1;
const int num_max_bounces = 2;
const float max_ray_distance = 100;
const vec4 sky_color = vec4(0.5, 0.5, 1.0, 1.0);
Lamp lamps[1];
vec4 default_initial_light = vec4(0.0, 0.0, 0.0, 1.0);
ChunkTraversals chunk_traverse;
float ray_margin = 0.0001;
int try_count = 0;

//// Prototype octree shader \\\\


// global octree
//uniform Octree g_oct_buf[MAX_OCTREE_ELEMENTS];


int octree_allocate(int parent)
{
    int out_index;
    /*
    int out_index = g_oct_buf[0].children[1];
    if (g_oct_buf[out_index].parent != -1)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (g_oct_buf[g_oct_buf[out_index].parent].children[i] == out_index)
            {
                g_oct_buf[g_oct_buf[out_index].parent].children[i] = -1;
            }
        }
    }
    */
    bool found = false;
    for (int i = 2; i < MAX_OCTREE_ELEMENTS; ++i)
    {
        if (g_oct_buf[i].parent == -1)
        {
            found = true;
            out_index = i;
            break;
        }
        else if (length(g_oct_buf[i].pos.xyz - eye) > 50 && g_oct_buf[i].size < 30)
        {
            out_index = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        return -1;
    }
    g_oct_buf[out_index].parent = parent;
    for (int i = 0; i < 8; ++i)
    {
        g_oct_buf[out_index].children[i] = -1;
    }
    //g_oct_buf[0].children[1] = (g_oct_buf[0].children[1] + 1);

    return out_index;
}

int octree_lookup(int depth, vec3 coord, vec4 in_val, bool allocate, bool set)
{
    int current_index = 0;
    int last_index = 0;
    for (int i = 0; i < depth; ++i)
    {
        vec3 node_center = g_oct_buf[current_index].pos.xyz + vec3(g_oct_buf[current_index].size/2.0);
        int x_index = int(coord.x > node_center.x);
        int y_index = int(coord.y > node_center.y);
        int z_index = int(coord.z > node_center.z);
        int child_index = x_index + y_index * 2 + z_index * 4;

        if (g_oct_buf[current_index].children[child_index] < 0)
        {
            if (allocate)
            {
                int new_index = octree_allocate(current_index);
                if (new_index == -1)
                {
                    return current_index;
                }
                g_oct_buf[current_index].children[child_index] = new_index;
                g_oct_buf[new_index].size =
                    g_oct_buf[current_index].size / 2.0;
                g_oct_buf[new_index].pos.xyz =
                    g_oct_buf[current_index].pos.xyz + vec3(x_index, y_index, z_index) *
                    g_oct_buf[current_index].size / 2.0;
            }
            else
            {
                return current_index;
            }
        }
        //else if (g_oct_buf[g_oct_buf[current_index].children[child_index]].value.x < -999)
        {
            //return current_index;
        }
        last_index = current_index;
        if (set)
        {
            vec4 sum_val = vec4(0);
            int count = 0;
            for (int j = 0; j < 8; ++j)
            {
                if (g_oct_buf[current_index].children[child_index] >= 0)
                {
                    sum_val += g_oct_buf[g_oct_buf[current_index].children[child_index]].value;
                    count += 1;
                }
            }
            g_oct_buf[current_index].value = sum_val/count;
            //g_oct_buf[current_index].parent = last_index;
        }
        current_index = g_oct_buf[current_index].children[child_index];
        
    }
    
    return current_index;
}

int octree_set(int depth, vec3 coord, valtype val)
{
    int current_index = octree_lookup(depth, coord, val, false, true);
    g_oct_buf[current_index].value = val;
    return current_index;
}

valtype octree_get(int depth, vec3 coord, inout int ind)
{
    int current_index = octree_lookup(depth, coord, vec4(0), false, false);
    ind = current_index;
    return vec4(g_oct_buf[current_index].value.xyz, g_oct_buf[current_index].size);
}
//// Octree \\\\\

//// CircularRayBuffer \\\\

struct CircularRayBuffer
{
    Ray rays[ARRAY_SIZE];
    int add_index;
    int process_index;
};

CircularRayBuffer ray_buffer;

void init_buffer()
{
    ray_buffer.add_index = 0;
    ray_buffer.process_index = 0;
}

int get_ray_count()
{
    return ray_buffer.add_index - ray_buffer.process_index;
    int counter = 0;
    int count = ray_buffer.process_index;
    while (count != ray_buffer.add_index)
    {
        count = (count + 1) % ARRAY_SIZE;
        counter += 1;
    }
    return counter;
}

int add_ray()
{
    int out_index = ray_buffer.add_index;
    ray_buffer.add_index = (ray_buffer.add_index + 1) % ARRAY_SIZE; 
    return out_index; 
}

int pop_front()
{
    ray_buffer.process_index = (ray_buffer.process_index + 1) % ARRAY_SIZE;
    if (ray_buffer.process_index == ray_buffer.add_index)
    {
        return -1;
    }
}

int proc_next_ray()
{
    int out_index = ray_buffer.process_index;
    return out_index;
}

//// CircularRayBuffer ////

vec4 get_tex_val(float x, float y, int tex_index)
{
    int x_int = int(x * tex_width);
    int y_int = int(y * tex_height);
    return textures[x_int + tex_width * y_int + tex_width * tex_height * tex_index];
}

vec4 get_mip_map1_val(float x, float y, int tex_index)
{
    int x_int = int(x * tex_width/(2*2));
    int y_int = int(y * tex_height/(2*2));
    return mip_map2[x_int + tex_width * y_int/(2*2) + tex_width * tex_height * tex_index/(2*2*2*2)];
}

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

int cube_type(vec3 in_pos, ChunkInfo in_chunk)
{
    return cube_states[(chunk_block_count * in_chunk.index +
        (int(floor(in_pos.x - in_chunk.coord.x))) +
        chunk_size_x * (int(floor(in_pos.y - in_chunk.coord.y))) +
        chunk_size_y * chunk_size_x * (int(floor(in_pos.z - in_chunk.coord.z))))];

}

vec2 intersect_box_scale_full(vec3 origin, vec3 dir, const vec3 box_origin, vec3 scale)
{
    vec3 b_min = box_origin;
    vec3 b_max = box_origin + scale;
    vec3 tMin = (b_min - origin) / dir;
    vec3 tMax = (b_max - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

vec2 intersect_box(vec3 origin, vec3 dir, const vec3 box_origin) {
    vec3 b_min = box_origin;
    vec3 b_max = box_origin + vec3(1.0);
    vec3 inv = 1.0 / dir;
    vec3 tMin = (b_min - origin) * inv;
    vec3 tMax = (b_max - origin) * inv;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear,tFar);
}

int get_map_chunk_index(ivec3 ref,
    ivec3 pos)
{
    ivec3 rel = pos - ref;
    ivec3 adj = pos;//% ivec3(chunk_scale);
    int out_index = adj.x + adj.y * chunk_map_size_x + adj.z * chunk_map_size_x * chunk_map_size_y;

    if (pos.y >= chunk_map_size_y || pos.x >= chunk_map_size_x || pos.z >= chunk_map_size_z ||
        pos.y < 0 || pos.x < 0 || pos.z < 0)
    {
        return -2;
    }

    return out_index;
}

int chunk_map_lookup(vec3 origin, vec3 dir, float start_d, inout ChunkTraversal ct)
{
    vec3 ref = floor(origin / chunk_scale);
    float d = 0;// ray_margin;
    float short_d = d;

    vec3 fake_origin = origin - floor(origin / chunk_scale) * chunk_scale;
    vec3 loc = origin + start_d * dir;
    vec3 fake_loc = fake_origin + start_d * dir;
    vec3 cur_chunk_coord = floor(loc / chunk_scale);
    vec3 cur_chunk_block_coord = cur_chunk_coord * chunk_scale;
    vec3 fake_cur_chunk_coord = floor(fake_loc / chunk_scale);
    vec3 fake_cur_chunk_block_coord = fake_cur_chunk_coord * chunk_scale;

    int index = get_map_chunk_index(ivec3(ref), ivec3(cur_chunk_coord));

    vec2 limits = intersect_box_scale_full(fake_origin,
        dir,
        fake_cur_chunk_block_coord,
        chunk_scale);

    ct.info.coord = vec4(cur_chunk_block_coord, 0.0);
    ct.info.index = chunk_map[index];
    ct.limits = limits;

    return index;
}

void chunks_map_lookup(vec3 origin, vec3 dir, int max_num)
{
    vec3 ref = floor(origin / chunk_scale);
    float d = 0;// ray_margin;
    float short_d = d;
    int add_count = 0;
    int i;
    for (i = 0; i < MAX_CHUNK_INFOS; ++i)
    {
        
        vec3 fake_origin = origin - floor(origin/chunk_scale)*chunk_scale;
        vec3 loc = origin + d * dir;
        vec3 fake_loc = fake_origin + d * dir;
        vec3 cur_chunk_coord = floor(loc / chunk_scale);
        vec3 cur_chunk_block_coord = cur_chunk_coord * chunk_scale;
        vec3 fake_cur_chunk_coord = floor(fake_loc / chunk_scale);
        vec3 fake_cur_chunk_block_coord = fake_cur_chunk_coord * chunk_scale;

        int index = get_map_chunk_index(ivec3(ref), ivec3(cur_chunk_coord));
        
        vec2 limits = intersect_box_scale_full(fake_origin,
            dir,
            fake_cur_chunk_block_coord,
            chunk_scale);
        
        chunk_traverse.infos[add_count].info.coord = vec4(cur_chunk_block_coord, 0.0);
        chunk_traverse.infos[add_count].info.index = chunk_map[index];
        chunk_traverse.infos[add_count].limits = limits;
        
        d = limits.y + ray_margin/100;
        short_d = limits.y - ray_margin/100;
        if (index < 0)
        {
            continue;
        }
        //chunk_traverse.infos[i] = new_traverse;
        add_count += 1;
    }
    chunk_traverse.count = add_count;// add_count;
    //return chunk_traverse;
}


void find_normal(inout HitInfo in_info)
{
    vec3 int_pos = in_info.position;
    vec3 dir = in_info.incident;
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
    in_info.side_index = 1;
    
    if (x_dist < y_dist && x_dist < z_dist)
    {
        norm = vec3(1.0, 0.0, 0.0);
        rel_up = vec3(0.0, 1.0, 0.0);
        x_tex = z_f;
        y_tex = 1 - y_f;
        in_info.side_index = 0;
    }
    if (z_dist < y_dist && z_dist < x_dist)
    {
        norm = vec3(0.0, 0.0, 1.0);
        rel_up = vec3(0.0, 1.0, 0.0);
        x_tex = x_f;
        y_tex = 1 - y_f;
        in_info.side_index = 0;
    }
    if (dot(dir, norm) > 0)
    {
        norm = -norm;
        if (in_info.side_index == 1)
        {
            in_info.side_index = 2;
        }
    }
    /*
    norm = (int_pos - in_info.cube_center);
    //norm = norm / max(max(norm.x, norm.y), norm.z);
    float abs_x = abs(norm.x);
    float abs_y = abs(norm.y);
    float abs_z = abs(norm.z);
    norm = vec3(sign(norm.x)*int(abs_x > abs_z && abs_x > abs_y),
        sign(norm.y)* int(abs_y > abs_z && abs_y > abs_x),
        sign(norm.z)* int(abs_z > norm.y && abs_z > abs_x));
    */

    in_info.tex_coords = vec2(x_tex, y_tex);
    in_info.normal = norm;
}

void gen_refraction(inout HitInfo in_hit, int ray_index, vec4 ray_color)
{
    ray_buffer.rays[ray_index].origin = in_hit.position -ray_margin * in_hit.normal / 100.0;
    ray_buffer.rays[ray_index].dir = in_hit.incident;
    ray_buffer.rays[ray_index].color = ray_buffer.rays[in_hit.ray_id].color * in_hit.refract_color;
}

void gen_reflection(inout HitInfo in_hit, int ray_index, vec4 ray_color)
{
    ray_buffer.rays[ray_index].origin = in_hit.position +in_hit.normal * ray_margin / 100.0;
    ray_buffer.rays[ray_index].dir = in_hit.incident - 2 * (dot(in_hit.incident, in_hit.normal)) * in_hit.normal;
    //ray_buffer.rays[ray_index].color = ray_buffer.rays[in_hit.ray_id].color*in_hit.reflect_color;// / (in_hit.distance * in_hit.distance + 1);
    float draw = 2 * random(mod(gl_GlobalInvocationID.xy * (try_count^ 1234), 1024) / vec2(1024, 1024)) - 1;
    float draw2 = 2 * random2(mod(gl_GlobalInvocationID.xy * ((try_count + 1) ^ 4356), 1024) / vec2(1024, 1024)) - 1;
    float draw3 = 2 * random3(mod(gl_GlobalInvocationID.xy * ((try_count + 2) ^ 7890), 1024) / vec2(1024, 1024)) - 1;
    float mix_val = 0.7;
    if (in_hit.block_type != 20)
    {
        ray_buffer.rays[ray_index].dir = normalize((ray_buffer.rays[ray_index].dir * (1 - mix_val) + in_hit.normal * mix_val) / 1.0 + 1.0 * normalize(vec3(draw, draw2, draw3)));
    }
}

/*
void test_intersect_boxes(vec3 origin, vec3 dir, float max_distance, inout HitInfo hit_info, bool pass_transparent)
{
    for (int k = 0; k < chunk_traverse.count; ++k)
    {
        if (chunk_traverse.infos[k].info.index < 0)
        {
            if (chunk_traverse.infos[k].info.index == -2)
            {
                return;
            }
            continue;
        }
        if (k == 0)
        {
            start_cube = floor(origin);
            prev_cube_type = cube_type(start_cube, chunk_traverse.infos[k].info);
        }
        if (chunk_traverse.infos[k].limits.x < 0)
        {
            start_loc = origin;
            start_d = 0;
            chunk_dist = chunk_traverse.infos[k].limits.y;
        }
        else
        {
            start_loc = origin + dir * (chunk_traverse.infos[k].limits.x);
            start_d = chunk_traverse.infos[k].limits.x;
            chunk_dist = chunk_traverse.infos[k].limits.y - (chunk_traverse.infos[k].limits.x);
        }

        float d = ray_margin;
        float short_d = -ray_margin;

        for (int i = 0; i < int(2 * chunk_size_x); ++i)
        {
            int block_type = cube_type();
            if (block_type != prev_cube_type)
            {
                // set hit
            }
            intersect_box();
        }
    }

}
*/

void intersect_boxes(vec3 origin, vec3 dir, float max_distance, int seed, inout HitInfo hit_info, bool pass_transparent)
{
    //HitInfo hit_info;
    hit_info.hit = false;
    hit_info.distance = -1;

    vec3 start_loc;

    // Lookup all the chunks the ray will pass through
    //chunks_map_lookup(origin, dir, int(max_distance / (chunk_size_x)) + 2);

    vec4 out_color = vec4(1.0);
    float min_total_d = 10000;
    float start_d = 0;
    float chunk_dist;
    // For each chunk the ray passes through
    // This implies each chunk's ray calculations
    // won't necessarily happen in order.
    int k = 0;
    float last_d = 0;
    float next_d = 0;
    vec3 start_cube;
    int prev_cube_type;

    ChunkTraversal ct;
    
    float current_d = 0;
    for (int k = 0; k < 20; ++k)
    {
        ct.info.index = -1;
        int retval = chunk_map_lookup(origin, dir, current_d, ct);
        current_d = ct.limits.y + 50*ray_margin;
        if (retval < 0)
        {
            if (retval == -2)
            {
                return;
            }
            continue;
        }

        if (k == 0)
        {
            start_cube = floor(origin);
            prev_cube_type = cube_type(start_cube, ct.info);
        }

        last_d = ct.limits.y;

        float d = 0;// 50 * ray_margin;
        float short_d = -1*ray_margin;
        float long_d = 50*ray_margin;

        // Set distance variables based on whether ray starts in
        // middle of chunk or not
        if (ct.limits.x < 0)
        {
            start_loc = origin;
            start_d = 0;
            chunk_dist = ct.limits.y;
        }
        else
        {
            start_loc = origin + dir * (ct.limits.x);
            start_d = (ct.limits.x);
            chunk_dist = ct.limits.y - (ct.limits.x);
        }

        // If the current distance is more than the already hit
        // minimum, give up on this chunk
        //if (start_d > min_total_d)
        //{
        //    continue;
        //}

        // iterate the intersected blocks intersected
        // inside the current chunk
        for (int i = 0; i < int(2 * chunk_size_x); ++i)
        {
            
            vec3 fake_start = (start_loc - floor(start_loc));
            vec3 rel_loc = fake_start + dir * float(long_d);
            vec3 rel_box = floor(rel_loc);
            vec3 loc = start_loc + dir * float(long_d);
            vec3 cur_box = floor(loc);
            int cube_type = cube_type(cur_box, ct.info);
            float total_d = start_d + long_d;
            // Should collect light pass right through water? Water could affect color.
            // If we decide it should, uncomment the end of this line
            if ((cube_type != prev_cube_type) &&  total_d <= min_total_d)// && (!pass_transparent || (cube_type !=0 && cube_type != 4)))
            {
                
                hit_info.position = start_loc + dir * float(short_d);
                hit_info.hit = true;
                if (hit_info.hit)
                {
                    hit_info.cube_center = cur_box + vec3(0.5, 0.5, 0.5);
                    hit_info.incident = dir;
                    hit_info.distance = float(total_d);
                    hit_info.block_type = cube_type;
                    find_normal(hit_info);
                    int tex_index = (cube_type - 1) * 3 + hit_info.side_index;
                    if (cube_type == 0)
                    {
                        hit_info.reflect_color = vec4(0.0, 0.0, 0.0, 0.0);
                        hit_info.refract_color = vec4(1.0, 1.0, 1.0, 1.0);
                    }
                    else if (cube_type == 21)
                    {
                        hit_info.reflect_color = vec4(10.0);
                        hit_info.refract_color = vec4(0, 0, 0, 0);
                    }
                    else if (cube_type != 20)
                    {
                        hit_info.reflect_color = get_tex_val(hit_info.tex_coords.x, hit_info.tex_coords.y, tex_index) * out_color;
                        hit_info.refract_color = vec4(0, 0, 0, 0);
                    }
                    else if (cube_type == 20)
                    {
                        float r0 = (1 - 1.33) / (1 + 1.33);
                        float fresnel_factor = ((r0 + (1 - r0) * pow(1 - dot(hit_info.incident, -hit_info.normal), 5)));
                        hit_info.reflect_color = vec4(0.5, 0.5, 0.5, 0.2)*(fresnel_factor);
                        hit_info.refract_color = vec4(1.0, 1.0, 1.0, 0.2);
                    }
                    
                    min_total_d = float(total_d);
                    break;
                }
            }
            if (cube_type == 20)
            {
                out_color *= vec4(0.83, 0.91, 0.95, 1.0);
            }
            
            prev_cube_type = cube_type;
            vec2 box_limits;
            box_limits = intersect_box(fake_start, dir, rel_box);
            d = box_limits[1];
            /*
            vec3 p = fake_start + dir * d;
            vec3 dv = p - round(p);
            dv = normalize(min(min(dv.x, dv.y), dv.z) / dv);
            dv = normalize(pow(dv, vec3(10, 10, 10)));
            float rd = min(ray_margin / abs(dot(dv, dir)), 1 0*ray_margin);
            */
            short_d = d;// -ray_margin;
            d = d;// +0.01 * (box_limits[1] - box_limits[0]);
            float min_d = d + 50*ray_margin;// 0.1 * (box_limits[1] - box_limits[0]);
            long_d = d + 2*ray_margin;// *(box_limits[1] - box_limits[0]);
            if (min_d >= chunk_dist || hit_info.hit)
            {
                break;
            }
        }
    }

    //return hit_info;
}

ivec2 create_rays_from_hit(inout HitInfo hit)
{
    int refract_ray = -1;
    int reflect_ray = -1;
    float fresnel_factor = 1.0;// dot(hit.incident, hit.normal)* hit.color.w;
    if (hit.refract_color.w > 0.01)
    {
        float r0 = (1 - 1.33) / (1 + 1.33);
        fresnel_factor = (1 - (r0 + (1-r0)*pow(1-dot(hit.incident, hit.normal), 5)));
        refract_ray = add_ray();
        gen_refraction(hit, refract_ray, ray_buffer.rays[hit.ray_id].color * hit.refract_color *(1 - fresnel_factor));
        //Populate refraction ray
    }

    reflect_ray = add_ray();
    gen_reflection(hit, reflect_ray, ray_buffer.rays[hit.ray_id].color*hit.reflect_color * fresnel_factor);

    // Populate reflect ray
    return ivec2(refract_ray, reflect_ray);
}

// in_loc is the location we want to collect light at
// in_direction is the direction of the ray that hit in_loc
// distance is the distance traversed by the ray before hitting in_loc
vec4 collect_light(vec3 in_loc, vec3 in_direction, float distance)
{
    HitInfo hit_info;
    vec4 csum = vec4(0.0, 0.0, 0.0, 1.0);
    int hit = 0;
    for (int i = 0; i < num_lamps; ++i)
    {
        float d = length(lamps[i].loc.xyz - in_loc) + distance;
        vec3 dir = normalize(lamps[i].loc.xyz - in_loc);
        //hit_info = intersect_boxes(in_loc, dir, 10000);
        intersect_boxes(in_loc, dir, 100, 0, hit_info, true);
        if (!hit_info.hit)
        {
            csum += max(0,dot(in_direction, dir)) * (lamps[i].color) * (lamps[i].intensity) / float(d * d);
        }
    }
    return csum;
}

void intersect_boxes_index(int in_index, inout HitInfo hit)
{
    intersect_boxes(ray_buffer.rays[in_index].origin, ray_buffer.rays[in_index].dir, 100, 0, hit, false);
    hit.ray_id = in_index;
}

// "hit" defined as a change in refractive index
// Assuming one initial ray for now
vec4 trace(vec3 origin, vec3 dir, inout vec4 norm, inout vec3 hit_loc)
{
    // Final output light. Ray results are added to this
    vec4 out_light = vec4(0.0, 0.0, 0.0, 1.0);

    // Set up and shoot first ray. This should never have to be repeated.
    vec4 init_color = vec4(0.0);
    int num_tries = 1;
    int dynamic_max_bounces = max_bounces;
    int dynamic_num_tries = num_tries;
    // Loop over initial rays per pixel
    for (int k = 0; k < 1; ++k)
    {
        try_count += 1;
        init_buffer();
        int initial_ray = add_ray();
        HitInfo initial_hit;
        HitInfo hit;
        
        ray_buffer.rays[initial_ray].origin = origin;
        ray_buffer.rays[initial_ray].dir = dir;
        ray_buffer.rays[initial_ray].color = vec4(1.0, 1.0, 1.0, 1.0);

        int count = 0;
        // Loop through the max number of "bounces".
        // Changing medium counts as a bounce. 
        for (int i = 0; i < 7; ++i)
        {
            // Process entirety of current ray buffer
            const int to_process_count = get_ray_count();
            for (int j = 0; j < to_process_count; ++j)
            {
                count += 1;
                int next_ray = proc_next_ray();
                intersect_boxes_index(next_ray, hit);
                if (i == 0)
                {
                    if (!(include_first_bounce > 0))
                    {
                        //if (hit.distance > 300)
                        {
                            dynamic_max_bounces = 1;// clamp(int(15 / log(hit.distance)), 2, 3);
                            dynamic_num_tries = 1;// clamp(int(14 / log(hit.distance)), 1, 20);
                        }
                    }
                    else
                    {
                        dynamic_num_tries = 1;
                    }
                    
                    if (hit.hit)
                    {
                        init_color = hit.reflect_color;
                        norm = vec4(hit.normal, hit.distance);
                        hit_loc = hit.position;
                    }
                    else
                    {
                        init_color = vec4(0.65, 0.7, 0.95, 1.0);
                        norm = vec4(0.0);
                    }
                    hit.distance = 0;
                    if (include_first_bounce > 0)
                    {
                        if (hit.hit)
                        {
                            return init_color;
                        }
                        else
                        {
                            return vec4(0.65, 0.7, 0.95, 1.0);
                        }
                    }
                }
                //intersect_boxes(ray_buffer.rays[next_ray].origin, ray_buffer.rays[next_ray].dir, 100, 0, hit);
                if (hit.hit)
                {
                    // Add reflection ray to queue
                    int reflection = add_ray();
                    gen_reflection(hit, reflection, vec4(1.0));
                    //ray_buffer.rays[reflection].origin = hit.position;
                    //ray_buffer.rays[reflection].dir = dir;
                    ray_buffer.rays[reflection].color = hit.reflect_color * (ray_buffer.rays[next_ray].color);// / (hit.distance * hit.distance + 1));
                    out_light += ray_buffer.rays[next_ray].color * hit.reflect_color * collect_light(ray_buffer.rays[reflection].origin, hit.normal, hit.distance);
                    if (max(hit.reflect_color.x, max(hit.reflect_color.y, hit.reflect_color.z)) > 1)
                    {
                        vec4 ref_color = ray_buffer.rays[next_ray].color * hit.reflect_color;
                        out_light += ref_color;
                        
                    }
                    if (hit.refract_color.w > 0.01)
                    {
                        // If material is transparent add refraction ray
                        int refraction = add_ray();
                        ray_buffer.rays[refraction].origin = hit.position + 2 * ray_margin * hit.incident;
                        ray_buffer.rays[refraction].dir = hit.incident; // for now, refraction direction = incident direction
                        ray_buffer.rays[refraction].color = ray_buffer.rays[next_ray].color;// hit.refract_color* (ray_buffer.rays[next_ray].color);/// / (hit.distance * hit.distance + 1));
                        out_light += ray_buffer.rays[next_ray].color * hit.refract_color * collect_light(ray_buffer.rays[refraction].origin, ray_buffer.rays[refraction].dir, hit.distance);
                    }
                }
                else
                {
                    out_light += ray_buffer.rays[next_ray].color * vec4(0.65, 0.7, 0.95, 1.0);
                }
                pop_front();
            }
            if (count >= max_bounces)
            {
                break;
            }
        }
    }
    if (!(include_first_bounce > 0))
    {
        return out_light/dynamic_num_tries - init_color;
    }
    return out_light/dynamic_num_tries;// vec4(0.1)* count;
    /*
    If hit :
        Add reflection ray to queue with color hit.reflect_color*ray_color*distance_factor
        Collect reflection light filtered through hit.reflect_color*ray_color*distance_factor
        If transparent :
            Add refraction ray to queue with color hit.refract_color*ray_color*distance_factor
            Collect refraction light filtered through hit.refract_color*ray_color*distance_factor
    For rays in queue
        Shoot ray
        If hit :
            Add reflection ray to queue with color reflect_color* ray_color* distance_factor
            Collect reflection light filtered through reflect_color* ray_color* distance_factor
            If transparent :
                Add refraction ray to queue with color refract_color* ray_color* distance_factor
                Collect refraction light filtered through refract_color* ray_color* distance_factor
    */
}

vec4 lerp(vec4 s, vec4 e, float t) { return s + (e - s) * t; }
vec4 blerp(vec4 c00, vec4 c10, vec4 c01, vec4 c11, float tx, float ty) {
    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}

void main()
{
    
    // temporary
    lamps[0].color = vec4(1.0, 1.0, 1.0, 1.0);
    lamps[0].loc = vec4(7000 * cos(1 * 2 * 3.14159 * (int(ttime + 150) % 300) / 600.0), 7000 * sin(1 * 2 * 3.14159 * (int(ttime + 150) % 300) / 600.0), 10, 0.0);
    //lamps[0].loc = vec4(7000, 7000, 10000, 0.0);
    //lamps[0].loc = vec3(5000, 1, 10000);
    lamps[0].intensity = 100000000;
    // end temporary

    uint chunk_num = gl_GlobalInvocationID.z;
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(out_tex);
    ivec2 hi_res_size = imageSize(out_tex);
    float draw, draw2, draw3;
    vec3 rand_vec = vec3(0);
    if (!(include_first_bounce > 0))
    {
        size = imageSize(out_tex_low_res);
        float draw = 2 * random(mod(gl_GlobalInvocationID.xy * ((int(ttime * 100) % 200) ^ 1234), 1024) / vec2(1024, 1024)) - 1;
        float draw2 = 2 * random2(mod(gl_GlobalInvocationID.xy * (((int(ttime * 100) % 200) + 1) ^ 4356), 1024) / vec2(1024, 1024)) - 1;
        float draw3 = 2 * random3(mod(gl_GlobalInvocationID.xy * (((int(ttime * 100) % 200) + 2) ^ 7890), 1024) / vec2(1024, 1024)) - 1;
        float mix_val = 0.7;
        //rand_vec = 0.075 * normalize(vec3(draw, draw2, draw3));
    }
    ivec2 low_res_size = imageSize(out_tex_low_res);
    ivec2 pix_low_res = ivec2(int(low_res_size.x * float(pix.x * 1.0) / size.x), int(low_res_size.y * float(pix.y * 1.0) / size.y));
    ivec2 pix_high_res = ivec2(int(hi_res_size.x * float(pix.x * 1.0) / low_res_size.x), int(hi_res_size.y * float(pix.y * 1.0) / low_res_size.y));
    vec2 pix_low_res_exact = vec2((low_res_size.x * float(pix.x * 1.0) / size.x), (low_res_size.y * float(pix.y * 1.0) / size.y));
    vec4 frag_norm;

    vec3 init_vec = vec3(1.0, 0.0, 0.0);

    int count = 0;

    if (pix.x >= size.x || pix.y >= size.y)
    {
        return;
    }
    vec2 pos = vec2(pix) / vec2(size.x, size.y);
    double pos_x = double(pix.x) / double(size.x);
    double pos_y = double(pix.y) / double(size.y);
    
    
    
    //vec4 high_res_norm = vec4(2.0);
    //if (!(include_first_bounce > 0))
    //{
    //    high_res_norm = imageLoad(norm_tex, pix_high_res);
    //}
    //if (length(high_res_norm.xyz) >= 0.9)
    {               
        vec3 dir = mix(mix(ray00, ray01, float(pos_y)), mix(ray10, ray11, pos.y), float(pos_x)) + rand_vec;
        dir = normalize(dir);
        vec4 norm;
        vec3 hit_loc;
        float reactivity = 0;
        vec4 ret = trace(eye, dir, norm, hit_loc);
        if (include_first_bounce > 0)
        {
            /*
            vec4 out_norm1 = imageLoad(norm_tex_low_res, pix_low_res);
            vec4 out_norm2 = imageLoad(norm_tex_low_res, pix_low_res + ivec2(0, 1));
            vec4 out_norm3 = imageLoad(norm_tex_low_res, pix_low_res + ivec2(1, 0));
            vec4 out_norm4 = imageLoad(norm_tex_low_res, pix_low_res + ivec2(1, 1));
            vec4 out_color1 = max(dot(out_norm1.xyz, norm.xyz), 0)*imageLoad(out_tex_low_res, pix_low_res);
            vec4 out_color2 = max(dot(out_norm2.xyz, norm.xyz), 0) * imageLoad(out_tex_low_res, pix_low_res + ivec2(0,1));
            vec4 out_color3 = max(dot(out_norm3.xyz, norm.xyz), 0) * imageLoad(out_tex_low_res, pix_low_res + ivec2(1, 0));
            vec4 out_color4 = max(dot(out_norm4.xyz, norm.xyz), 0) * imageLoad(out_tex_low_res, pix_low_res + ivec2(1, 1));
            */
            int r = clamp(int(200/norm.w),2,10);
            int x = 0;
            int y = 0;
            vec4 out_color = vec4(0.0);
            float factors = 0;
            int oct_depth;// = int(clamp(1000000 / (length(eye - hit_loc) * length(eye - hit_loc)), 1, 10));
            float dist = length(eye - hit_loc);
            if (dist < 10)
            {
                oct_depth = 15;
            }
            else if (dist < 20)
            {
                oct_depth = 14;
            }
            else if (dist < 50)
            {
                oct_depth = 10;
            }
            else if (dist < 200)
            {
                oct_depth = 2;
            }
            else
            {
                oct_depth = 2;
            }
            /*
            for (int i = y - r; i < y + r; i++) {
                for (int j = x; (j - x) * (j - x) + (i - y) * (i - y) <= r * r; j--) {
                    vec4 out_norm1 = imageLoad(norm_tex_low_res, pix_low_res + ivec2(i, j));
                    vec4 out_color1 = octree_get(oct_depth, hit_loc); //imageLoad(out_tex_low_res, pix_low_res + ivec2(i, j));
                    vec4 out_pos1 = imageLoad(pos_tex, pix_low_res + ivec2(i, j));
                    float dist = length(pix_low_res + ivec2(i, j) - pix_low_res_exact) + 1;
                    float factor = int(length(out_pos1.xyz*norm.xyz - hit_loc.xyz*norm.xyz) < 0.1)*max(dot(out_norm1.xyz, norm.xyz), 0) / dist;
                    factors += factor;
                    out_color += out_color1 * factor;
                }
                for (int j = x + 1; (j - x) * (j - x) + (i - y) * (i - y) <= r * r; j++) {
                    vec4 out_norm1 = imageLoad(norm_tex_low_res, pix_low_res + ivec2(i, j));
                    vec4 out_color1 = octree_get(oct_depth, hit_loc); //imageLoad(out_tex_low_res, pix_low_res + ivec2(i, j));
                    vec4 out_pos1 = imageLoad(pos_tex, pix_low_res + ivec2(i, j));
                    float dist = length(pix_low_res + ivec2(i, j) - pix_low_res_exact) + 1;
                    float factor = int(length(out_pos1.xyz * norm.xyz - hit_loc.xyz * norm.xyz) < 0.1) * max(dot(out_norm1.xyz, norm.xyz), 0) / dist;
                    factors += factor;
                    out_color += out_color1 * factor;
                }
            }
            out_color /= factors;
            */
            if (oct_depth < 10)
            {
                out_color = imageLoad(out_tex_low_res, pix_low_res);
            }
            else
            {
                /*
                vec4 vect1 = normalize(vec4(int(norm.x == 0), int(norm.y == 0), int(norm.z == 0), 0));
                vec4 vect2 = vec4(cross(norm.xyz, vect1.xyz),0);
                vec4 vect3 = (vect1 + vect2) / 2.0;
                vec4 vect4 = vec4(cross(vect3.xyz, norm.xyz),0);
                */
                int ind = -1;
                out_color = imageLoad(out_tex_low_res, pix_low_res);// +ivec2(0, 0));
                //vec4 out_color2 = imageLoad(out_tex_low_res, pix_low_res + ivec2(1, 0));
                //vec4 out_color3 = imageLoad(out_tex_low_res, pix_low_res + ivec2(0, 1));
                //vec4 out_color4 = imageLoad(out_tex_low_res, pix_low_res + ivec2(1, 1));
                //out_color = blerp(out_color1, out_color2, out_color3, out_color4,
                //    pix_low_res_exact.x - pix_low_res.x,
                //    pix_low_res_exact.y - pix_low_res.y);
                //out_color = octree_get(oct_depth, hit_loc, ind);
                /*
                vec4 center = g_oct_buf[ind].pos + vec4(g_oct_buf[ind].size / 2.0);
                vec4 out_color1 = octree_get(oct_depth, (center + vect3*out_color.w).xyz, ind);
                vec4 out_color2 = octree_get(oct_depth, (center - vect3 * out_color.w).xyz, ind);
                vec4 out_color3 = octree_get(oct_depth, (center + vect4 * out_color.w).xyz, ind);
                vec4 out_color4 = octree_get(oct_depth, (center - vect4 * out_color.w).xyz, ind);
                out_color = out_color + blerp(out_color2, out_color4, out_color1, out_color3,
                    length((hit_loc - (center + vect3* out_color.w).xyz)) / (out_color.w*2),
                    length(hit_loc - (center + vect4* out_color.w).xyz) / (out_color.w*2));
                    */
                /*
                if (abs(out_color.w - out_color1.w) < out_color.w/10.0)
                {
                    out_color = vec4(out_color1.xyz, out_color.w);
                }
                else
                {
                    out_color = vec4(-1,-1,-1, out_color.w);
                }
                */
                if (out_color.w > 1.0)
                {
                    //out_color = vec4(0);
                }
            }
            out_color.w = dist;
            
        
            imageStore(out_tex_bounce_pass, pix, out_color);
            imageStore(norm_tex, pix, norm);
            imageStore(out_tex, pix, ret + out_color); // log(1.5 * ret + 1));
        }
        else
        {
            int oct_depth;// = int(clamp(1000000 / (length(eye - hit_loc) * length(eye - hit_loc)), 1, 10));
            float dist = length(eye - hit_loc);
            if (dist < 10)
            {
                oct_depth = 15;
            }
            else if (dist < 20)
            {
                oct_depth = 14;
            }
            else if (dist < 50)
            {
                oct_depth = 10;
            }
            else if (dist < 200)
            {
                oct_depth = 2;
            }
            else
            {
                oct_depth = 2;
            }
            vec4 avg = vec4(0);
            if (false)//length(norm) > 0.1)
            {
                int index = octree_lookup(oct_depth, hit_loc, ret, false, false);
                float factor = 0;
                if (g_oct_buf[index].size < 1.0)
                {
                    factor = 0.05;
                }
                avg = (ret * (1 - factor) + g_oct_buf[index].value * factor);

                octree_set(oct_depth, hit_loc, avg);
                //octree_set(oct_depth, hit_loc, vec4(hit_loc, 0) / 1000.0);
            }
            //int ind = -1;
            //vec4 oct = octree_get(oct_depth - 4, hit_loc, ind);
            imageStore(out_tex_low_res, pix, log(1.5 * ret + 1));
            imageStore(norm_tex_low_res, pix, norm);
            imageStore(pos_tex, pix, vec4(hit_loc, 0));
            //imageStore(out_tex, pix_high_res, out_color1);
        }
    }
}
