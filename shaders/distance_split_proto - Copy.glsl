#version 460 core

//layout(local_size_x = 32, local_size_y = 32, local_size_z = 5) in;

#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16
#define GROUP_SIZE_Z 1
#define GROUP_ARRAY_SIZE GROUP_SIZE_X*GROUP_SIZE_Y
#define GROUP_ARRAY_SIZE_3D GROUP_SIZE_X*GROUP_SIZE_Y*GROUP_SIZE_Z


layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;

struct ChunkInfo
{
    vec3 coord;
    int index;
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
    ChunkInfo chunks[];
};
layout(binding = 1, rgba32f) uniform image2D out_tex;

// The camera specification
uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;
uniform int time;
uniform int cube_count_x;
uniform int cube_count_y;
uniform int cube_count_z;
uniform int tex_width;
uniform int tex_height;
uniform int chunk_count;
uniform int chunk_size_x;
uniform int chunk_size_y;
uniform int chunk_size_z;

struct IntersectProps
{
    vec3 normal;
    vec2 local_coord;
    vec3 reflection;
};

struct HitInfo
{
    vec4 color;
    IntersectProps props;
    vec3 loc;
    float distance;
    bool hit;
};

struct ChunkTraversal
{
    ChunkInfo info;
    float limit;
};

struct BeamReturn
{
    vec4 color;
    float d;
};

struct BlockIntersect
{
    float near;
    float far;
};

struct Lamp
{
    vec3 loc;
    vec4 color;
    float intensity;
};

int chunk_block_count = chunk_size_x * chunk_size_y * chunk_size_z;
const int num_lamps = 1;
const int num_initial_shots_from_pixel = 1;
const int num_max_bounces = 2;
const float max_ray_distance = 100;
const vec4 sky_color = vec4(0.8, 0.8, 1.0, 1.0);
Lamp lamps[1];
vec4 default_initial_light = vec4(0.0, 0.0, 0.0, 1.0);
shared float min_dists[GROUP_ARRAY_SIZE];
shared BeamReturn returns[GROUP_ARRAY_SIZE];

void set_beam_return(int x, int y, int z, BeamReturn ret)
{
    returns[x + GROUP_SIZE_X * y + GROUP_SIZE_X * GROUP_SIZE_Y * z] = ret;
}

BeamReturn get_beam_return(int x, int y, int z)
{
    return returns[x + GROUP_SIZE_X * y + GROUP_SIZE_X * GROUP_SIZE_Y * z];
}

vec4 get_tex_val(float x, float y, int tex_index)
{
    int x_int = int(x * tex_width);
    int y_int = int(y * tex_height);
    return textures[x_int + tex_width * y_int + tex_width * tex_height * tex_index];
}

IntersectProps get_intersect_props(vec3 int_pos, vec3 dir, int cube_type)
{
    IntersectProps out_props;
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
        y_tex = 1 - y_f;
        tex_index = 0;
    }
    if (z_dist < y_dist && z_dist < x_dist)
    {
        norm = vec3(0.0, 0.0, 1.0);
        rel_up = vec3(0.0, 1.0, 0.0);
        x_tex = x_f;
        y_tex = 1 - y_f;
        tex_index = 0;
    }

    if (dot(dir, norm) > 0)
    {
        norm = -norm;
    }
    out_props.normal = norm;
    out_props.local_coord = vec2(x_tex, y_tex);
    out_props.reflection = dir - 2 * (dot(dir, norm)) * norm;
    return out_props;
}

int cube_type(vec3 in_pos, ChunkInfo in_chunk)
{
        return cube_states[chunk_block_count * in_chunk.index +
            int(floor(in_pos.x - in_chunk.coord.x)) +
            chunk_size_x * int(floor(in_pos.y - in_chunk.coord.y)) +
            chunk_size_y * chunk_size_x * int(floor(in_pos.z - in_chunk.coord.z))];

}

float intersect_box_scale(vec3 origin, vec3 dir, const vec3 box_origin, vec3 scale)
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

float intersect_box(vec3 origin, vec3 dir, const vec3 box_origin)
{
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

ChunkTraversal chunk_lookup(vec3 origin, vec3 dir, float dist)
{
    ChunkTraversal out_traverse;
    vec3 in_pos = origin + dir * dist;
    for (int i = 0; i < chunk_count; ++i)
    {
        //chunk_alloc default_alloc2;
        //default_alloc2.mem_index = 0;
        //return default_alloc2;
        if (chunks[i].index > chunk_count)
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
            out_traverse.info = chunks[i];
            out_traverse.limit = intersect_box_scale(origin, 
                dir, 
                chunks[i].coord, 
                vec3(chunk_size_x, chunk_size_y, chunk_size_z));
            return out_traverse;
        }
    }
    out_traverse.limit = -1;
    return out_traverse;
}

BlockIntersect get_chunk_intersect(vec3 origin, vec3 dir, uint chunk_num)
{
    BlockIntersect out_intersect;
    ChunkTraversal chunk_traverse;
    out_intersect.far = 0;
    for (int i = 0; i <= chunk_num; ++i)
    {
        out_intersect.near = out_intersect.far;
        chunk_traverse = chunk_lookup(origin, dir, out_intersect.far + 0.0001);
        out_intersect.far = chunk_traverse.limit;
    }
    return out_intersect;
}

HitInfo intersect_boxes(vec3 origin, vec3 dir, float max_distance)
{
    HitInfo hit_info;
    hit_info.hit = false;
    float d = 0;
    float short_d = 0;
    ChunkTraversal chunk_traverse = chunk_lookup(origin, dir, d);
    if (chunk_traverse.limit < 0)
    {
        return hit_info;
    }

    vec4 out_color = vec4(1.0);
    for (int i = 0; i < 100; ++i)
    {
        vec3 loc = origin + dir * d;
        vec3 cur_box = floor(loc);
        if (d > chunk_traverse.limit)
        {
            chunk_traverse = chunk_lookup(origin, dir, d);
        }
        if (chunk_traverse.limit >= 0)
        {
            int cube_type = cube_type(loc, chunk_traverse.info);
            if (cube_type != 0)
            {
                hit_info.loc = origin + dir * short_d;
                hit_info.props = get_intersect_props(hit_info.loc, dir, cube_type);
                hit_info.color = get_tex_val(hit_info.props.local_coord.x, hit_info.props.local_coord.y, 0);
                hit_info.hit = true;
                hit_info.distance = d;
                break;
            }
        }
        else
        {
            d = chunk_traverse.limit + 0.001;
            short_d = chunk_traverse.limit - 0.001;
            continue;
        }
        d = intersect_box(origin, dir, cur_box);
        if (d > max_distance)
        {
            break;
        }
    }
    return hit_info;
}

// in_loc is the location we want to collect light at
// in_direction is the direction of the ray that hit in_loc
// distance is the distance traversed by the ray before hitting in_loc
vec4 collect_light(vec3 in_loc, vec3 in_direction, float distance)
{
    HitInfo hit_info;
    vec4 csum = vec4(0.0,0.0,0.0,1.0);
    for (int i = 0; i < num_lamps; ++i)
    {
        hit_info.hit = false;
        float d = length(lamps[i].loc - in_loc) + distance;
        vec3 dir = normalize(lamps[i].loc - in_loc);
        hit_info = intersect_boxes(in_loc, dir, 10000);
        if (!hit_info.hit)
        {
            csum += (lamps[i].color) * (lamps[i].intensity) / (d * d);
        }
    }
    return csum;
}

BeamReturn bounce_ray(vec3 origin, vec3 dir, float distance)
{
    BeamReturn out_ret;

    vec4 out_light = default_initial_light;
    HitInfo initial_hit = intersect_boxes(origin, dir, distance);
    out_ret.d = initial_hit.distance;
    out_ret.color = vec4(0.0,0.0,0.0,1.0);

    if (initial_hit.hit)
    {
        out_ret.color = vec4(1.0);
        return out_ret;
        vec4 running_color = initial_hit.color;
        float total_distance = 0;
        out_light += running_color * collect_light(initial_hit.loc,
            dir,
            total_distance);
        for (int i = 0; i < num_initial_shots_from_pixel; ++i)
        {
            HitInfo current_hit = initial_hit;
            for (int j = 0; j < num_max_bounces; ++j)
            {
                current_hit = intersect_boxes(current_hit.loc,
                    current_hit.props.reflection, 
                    max_ray_distance);
                running_color *= current_hit.color;
                total_distance += current_hit.distance;
                out_light += collect_light(current_hit.loc, current_hit.props.reflection, total_distance);
            }
        }
    }
    else
    {
        out_light = sky_color;
    }
    out_ret.color = out_light;
    return out_ret;
}

void main()
{
    // temporary
    lamps[0].color = vec4(1.0);
    lamps[0].loc = vec3(5000, 10000, 10000);
    lamps[0].intensity = 100000;
    // end temporary

    uint chunk_num = gl_GlobalInvocationID.z;
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(out_tex);

    vec2 pos = vec2(pixel) / vec2(size.x, size.y);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);

    //BlockIntersect chunk_distance = get_chunk_intersect(eye, dir, chunk_num);

    //vec3 start_loc = eye + chunk_distance.near*dir;
    BeamReturn ret = bounce_ray(eye, dir,
        100);// chunk_distance.far - chunk_distance.near);
    imageStore(out_tex, pixel, ret.color);
    return;
    //barrier();
    if (chunk_num == 0)
    {
        BeamReturn min_ret = get_beam_return(pixel.x, pixel.y, 0);
        for (int i = 1; i < GROUP_SIZE_Z; ++i)
        {
            BeamReturn cur_ret = get_beam_return(pixel.x, pixel.y, i);
            if (cur_ret.d < min_ret.d)
            {
                min_ret = cur_ret;
            }
        }
        imageStore(out_tex, pixel, min_ret.color);
    }
}
