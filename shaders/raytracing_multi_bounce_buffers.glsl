#version 460 core

#define GROUP_SIZE_X 32
#define GROUP_SIZE_Y 32
#define GROUP_SIZE_Z 1
#define GROUP_ARRAY_SIZE GROUP_SIZE_X*GROUP_SIZE_Y
#define GROUP_ARRAY_SIZE_3D GROUP_SIZE_X*GROUP_SIZE_Y*GROUP_SIZE_Z
#define MAX_CHUNK_INFOS 20

layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;

struct chunk_alloc
{
    vec3 coord;
    int mem_index;
};

struct ChunkInfo
{
    vec4 coord;
    int index;
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
layout(std430, binding = 6) buffer layoutName5
{
    ChunkInfo chunks[];
};

layout(binding = 1, rgba32f) uniform image2D out_tex;

struct BeamReturn
{
    vec4 color;
    float d;
};

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
uniform int chunk_count;
uniform int chunk_size_x;
uniform int chunk_size_y;
uniform int chunk_size_z;
uniform int chunk_map_size_x;
uniform int chunk_map_size_y;
uniform int chunk_map_size_z;

struct Lamp
{
    vec4 loc;
    vec4 color;
    float intensity;
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

vec3 light_pos = vec3(5000, 10000, 10000);
int chunk_block_count = chunk_size_x * chunk_size_y * chunk_size_z;
vec3 chunk_scale = vec3(chunk_size_x, chunk_size_y, chunk_size_z);
const int num_lamps = 1;
const int num_initial_shots_from_pixel = 1;
const int num_max_bounces = 1;
const float max_ray_distance = 100;
const vec4 sky_color = vec4(0.5, 0.5, 1.0, 1.0);
Lamp lamps[1];
vec4 default_initial_light = vec4(0.0, 0.0, 0.0, 1.0);
ChunkTraversals chunk_traverse;
float ray_margin = 0.001;

struct IntersectProps
{
    //vec3 loc;
    vec3 normal;
    vec2 local_coord;
    vec3 reflection;
    int tex_index;
    vec4 color;
    bool hit;
};

struct HitInfo
{
    //vec4 color;
    IntersectProps props;
    vec3 loc;
    float distance;
    bool hit;
    int block_type;
};


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
    return textures[x_int + tex_width * y_int + tex_width * tex_height * tex_index];
}


float intersectBox(vec3 origin, vec3 dir, const vec3 box_origin) {
    //vec3 fake_start = (origin - floor(origin));
    //vec3 rel_loc = fake_start + (in_box_origin - origin);
    //vec3 box_origin = floor(rel_loc);
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

// get_intersect_props fills in the IntersectProps structure of a ray hit based
// on what type of cube and other raytracing settings
// - input: int_pos is the reflection point outside of the hit cube
// - input: dir is the direction of the hitting ray, not the reflection
// - input: seed randomness seed
// - output: out_props the properties to fill in
void get_intersect_props(vec3 int_pos, vec3 dir, int cube_type, int seed, out IntersectProps out_props)
{
    out_props.hit = false;
    seed = seed + 1;
    //IntersectProps out_props;
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
        if (tex_index == 1)
        {
            tex_index = 2;
        }
    }
    out_props.local_coord = vec2(x_tex, y_tex);
    out_props.tex_index = (cube_type - 1) * 3 + tex_index;
    out_props.color = get_tex_val(out_props.local_coord.x, out_props.local_coord.y, out_props.tex_index);
    out_props.normal = norm;

    //out_props.normal = normalize(norm + 0.1 * (out_props.color.x - 1.0) * (rel_up));
    out_props.reflection = dir - 2 * (dot(dir, norm)) * norm;
    float draw = 2 * random(mod(gl_GlobalInvocationID.xy * (seed ^ 1234), 1024) / vec2(1024, 1024)) - 1;
    float draw2 = 2 * random2(mod(gl_GlobalInvocationID.xy * (seed ^ 4356), 1024) / vec2(1024, 1024)) - 1;
    float draw3 = 2 * random3(mod(gl_GlobalInvocationID.xy * (seed ^ 7890), 1024) / vec2(1024, 1024)) - 1;
    if (cube_type == 4)
    {
        float prob = dot(-normalize(dir), norm);
        if ((draw + 1) / 2.0 < prob)
        {
            out_props.hit = false;
        }
        else
        {
            out_props.hit = true;
        }
        out_props.color = vec4(0.2, 0.3, 0.5, 1.0);
        float mix_val = 0.0001;
        out_props.reflection = normalize((out_props.reflection * (1 - mix_val) + norm * mix_val) / 1.0 + 0.0001 * normalize(vec3(draw, draw2, draw3)));
        //out_props.hit = true;
        //out_props.hit = false;
    }
    else if (cube_type > 0)
    {
        
        float mix_val = 0.4;
        out_props.reflection = normalize((out_props.reflection * (1 - mix_val) + norm * mix_val) / 1.0 + 0.2 * normalize(vec3(draw, draw2, draw3)));
        out_props.hit = true;
    }
    //return out_props;
}

int cube_type(vec3 in_pos, ChunkInfo in_chunk)
{
    /*
    if (floor(in_pos.y - in_chunk.coord.y) < 8 && floor(in_pos.y - in_chunk.coord.y) > 4)
    {
    return 1;
    }
    return 0;
    
    if (in_pos.y < 5)
    {
        return 1;
    }
    return 0;
    */
    return cube_states[(chunk_block_count * in_chunk.index +
        (int(floor(in_pos.x - in_chunk.coord.x))) +
        chunk_size_x * (int(floor(in_pos.y - in_chunk.coord.y))) +
        chunk_size_y * chunk_size_x * (int(floor(in_pos.z - in_chunk.coord.z))))];

}

float intersect_box_scale(vec3 origin, vec3 dir, const vec3 box_origin, vec3 scale)
{
    vec3 b_min = box_origin;
    vec3 b_max = box_origin + scale;
    vec3 tMin = (b_min - origin) / dir;
    vec3 tMax = (b_max - origin) / dir;
    //vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    //float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tFar;
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

int get_map_chunk_index(ivec3 ref,
    ivec3 pos)
{
    ivec3 rel = pos - ref;
    ivec3 adj = pos%ivec3(chunk_scale);
    int out_index = adj.x + adj.y * chunk_map_size_x + adj.z * chunk_map_size_x * chunk_map_size_y;
    
    if (pos.y >= chunk_map_size_y || pos.x >= chunk_map_size_x || pos.z >= chunk_map_size_z ||
        pos.y < 0 || pos.x < 0 || pos.z < 0)
    {
        return -2;
    }

    return out_index;
}

void chunks_lookup(vec3 origin, vec3 dir, int max_num)
{
    int found_count = 0;
    for (int i = 0; i < chunk_count; ++i)
    {
        vec2 limits = intersect_box_scale_full(origin,
            dir,
            chunks[i].coord.xyz,
            vec3(chunk_size_x, chunk_size_y, chunk_size_z));
        if (limits.y > 0 && limits.x < limits.y)
        {
            ChunkTraversal new_traverse;
            new_traverse.info = chunks[i];
            new_traverse.limits = limits;
            chunk_traverse.infos[found_count] = new_traverse;
            found_count += 1;
        }
        if (found_count >= MAX_CHUNK_INFOS)
        {
            break;
        }
    }
    chunk_traverse.count = found_count;
    //return out_traverse;
}

ChunkTraversal chunk_lookup(vec3 origin, vec3 dir, float dist)
{
    ChunkTraversal out_traverse;
    vec3 in_pos = origin + dir * float(dist);
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
            out_traverse.limits.y = float(intersect_box_scale(origin,
                dir,
                chunks[i].coord.xyz,
                vec3(chunk_size_x, chunk_size_y, chunk_size_z)));
            return out_traverse;
        }
    }
    vec3 chunk_origin = vec3(chunk_size_x * floor(in_pos.x / chunk_size_x),
        chunk_size_y * floor(in_pos.y / chunk_size_y),
        chunk_size_z * floor(in_pos.z / chunk_size_z));
    out_traverse.limits.y = float(intersect_box_scale(origin,
        dir,
        chunk_origin,
        vec3(chunk_size_x, chunk_size_y, chunk_size_z)));
    out_traverse.info.index = -1;
    return out_traverse;
}


void chunks_map_lookup(vec3 origin, vec3 dir, int max_num)
{
    vec3 ref = floor(origin / chunk_scale);
    float d = 0;// ray_margin;
    float short_d = d;
    int add_count = 0;
    for (int i = 0; i < 20; ++i)
    {
        vec3 loc = origin + d * dir;
        vec3 cur_chunk_coord = floor(loc / chunk_scale);
        vec3 cur_chunk_block_coord = cur_chunk_coord * chunk_scale;
        
        int index = get_map_chunk_index(ivec3(ref), ivec3(cur_chunk_coord));
        
        vec2 limits = intersect_box_scale_full(origin,
            dir,
            cur_chunk_block_coord,
            chunk_scale);
        ChunkTraversal new_traverse;

        new_traverse.info.coord = vec4(cur_chunk_block_coord, 0.0);
        new_traverse.info.index = chunk_map[index];
        new_traverse.limits = limits;

        d = limits.y + ray_margin;
        short_d = limits.y - ray_margin;
        if (index < 0)
        {
            continue;
        }
        chunk_traverse.infos[add_count] = new_traverse;
        add_count += 1;
    }
    chunk_traverse.count = add_count;
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

void intersect_boxes(vec3 origin, vec3 dir, float max_distance, int seed, out HitInfo hit_info)
{
    //HitInfo hit_info;
    hit_info.hit = false;
    hit_info.distance = -1;
    
    vec3 start_loc;
    
    // Lookup all the chunks the ray will pass through
    chunks_map_lookup(origin, dir, int(max_distance/(chunk_size_x)) + 2);

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
    int prev_cube_type = 0;
    for (int m = 0; m < chunk_traverse.count; ++m)
    {
        k = m;
        /*
        for (int n = 0; n < chunk_traverse.count; ++n)
        {
            if (chunk_traverse.infos[n].limits.y > last_d)
            {
                k = n;
                break;
            }
        }
        for (int n = 0; n < chunk_traverse.count; ++n)
        {
            if (chunk_traverse.infos[n].limits.y < chunk_traverse.infos[k].limits.y && chunk_traverse.infos[n].limits.y > last_d)
            {
                k = n;
            }
        }
        */
        if (chunk_traverse.infos[k].info.index < 0)
        {
            if (chunk_traverse.infos[k].info.index == -2)
            {
                return;
            }
            continue;
        }
        last_d = chunk_traverse.infos[k].limits.y;

        float d = ray_margin;
        float short_d = -ray_margin;

        // Set distance variables based on whether ray starts in
        // middle of chunk or not
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

        // If the current distance is more than the already hit
        // minimum, give up on this chunk
        if (start_d > min_total_d)
        {
            continue;
        }

        // iterate the intersected blocks intersected
        // inside the current chunk
        for (int i = 0; i < int(2*chunk_size_x); ++i)
        {
            vec3 fake_start = (start_loc - floor(start_loc));
            vec3 rel_loc = fake_start + dir * d;
            vec3 rel_box = floor(rel_loc);
            vec3 loc = start_loc + dir*d;
            vec3 cur_box = floor(loc);
            int cube_type = cube_type(cur_box, chunk_traverse.infos[k].info);
            float total_d = start_d + d;
            if (cube_type != 0 && !(cube_type == 4 && prev_cube_type == 4) && total_d <= min_total_d)
            {
                hit_info.loc = start_loc + dir * float(short_d);
                get_intersect_props(hit_info.loc, dir, cube_type, seed, hit_info.props);

                hit_info.hit = true;
                
                hit_info.hit = hit_info.props.hit;
                if (hit_info.hit)
                {
                    hit_info.distance = float(total_d);
                    hit_info.block_type = cube_type;
                    hit_info.props.color *= out_color;
                    min_total_d = float(total_d);
                    break;
                }
            }
            if (cube_type == 4)
            {
                out_color *= vec4(0.95, 0.98, 1.0, 1.0);
            }
            prev_cube_type = cube_type;
            d = intersectBox(fake_start, dir, rel_box);
            short_d = d - ray_margin;
            d = d + ray_margin;
            if (d >= chunk_dist)
            {
                break;
            }
        }
    }
    //return hit_info;
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
        intersect_boxes(in_loc, dir, 100, 0, hit_info);
        if (!hit_info.hit)
        {
            csum += dot(in_direction, dir)*(lamps[i].color)*(lamps[i].intensity) / float(d * d);
        }
    }
    return csum;
}

BeamReturn trace(vec3 origin, vec3 dir, float max_dist)
{
    BeamReturn out_ret;
    vec4 out_light = vec4(0.0);// default_initial_light;
    
    HitInfo initial_hit;
    intersect_boxes(origin, dir, max_dist, 0, initial_hit);
    out_ret.d = initial_hit.distance;
    out_ret.color = vec4(0.0, 0.0, 0.0, 1.0);

    if (initial_hit.hit)
    {
        out_ret.color = vec4(1.0, 0, 0, 0);
        return out_ret;
        vec4 running_color = initial_hit.props.color;
        float total_distance = 0;
        HitInfo current_hit;
        out_light += running_color * collect_light(initial_hit.loc,
            initial_hit.props.normal,
            total_distance);
        for (int i = 0; i < num_initial_shots_from_pixel; ++i)
        {
            total_distance = 0;
            running_color = initial_hit.props.color;
            current_hit = initial_hit;
            get_intersect_props(current_hit.loc, current_hit.props.reflection, current_hit.block_type, i, current_hit.props);
            HitInfo next_hit;
            for (int j = 0; j < num_max_bounces; ++j)
            {
                intersect_boxes(current_hit.loc, current_hit.props.reflection, 100, (i)*int(ttime*1000), next_hit);
                if (next_hit.hit)
                {
                    running_color *= next_hit.props.color;
                    total_distance += next_hit.distance;
                    out_light += 1.0*running_color * collect_light(next_hit.loc, next_hit.props.reflection, total_distance)/ num_initial_shots_from_pixel;
                    current_hit = next_hit;
                }
                else// if (j > 0)
                {
                    break;
                }
            }
            out_light += running_color * sky_color / float(total_distance * total_distance + 1.5) / num_initial_shots_from_pixel;
            
            
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
    lamps[0].color = vec4(1.0,1.0,1.0,1.0);
    lamps[0].loc = vec4(7000 * cos(0.1*2 * 3.14159 * (int(75)%100)/100.0), 7000*sin(0.1*2*3.14159*(int(75) %100)/100.0), 10000, 0.0);
    lamps[0].loc = vec4(7000, 7000, 10000, 0.0);
    //lamps[0].loc = vec3(5000, 1, 10000);
    lamps[0].intensity = 200000000;
    // end temporary

    uint chunk_num = gl_GlobalInvocationID.z;
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
    double pos_x = double(pix.x) / double(size.x);
    double pos_y = double(pix.y) / double(size.y);
    vec3 dir = mix(mix(ray00, ray01, float(pos_y)), mix(ray10, ray11, pos.y), float(pos_x));

    dir = normalize(dir);

    /*
    vec3 in_pos = eye;
    float limit = 0.0001;
    for (int i = 0; i <= chunk_num; ++i)
    {
        vec3 chunk_origin = vec3(chunk_size_x * floor(in_pos.x / chunk_size_x),
            chunk_size_y * floor(in_pos.y / chunk_size_y),
            chunk_size_z * floor(in_pos.z / chunk_size_z));
        limit = intersect_box_scale(in_pos,
            dir,
            chunk_origin,
            vec3(chunk_size_x, chunk_size_y, chunk_size_z));
        in_pos = in_pos + dir * (limit + 0.001);
    }
    */

    BeamReturn ret = trace(eye, dir, 1000);
    //set_beam_return(pix.x, pix.y, int(chunk_num), ret);
    imageStore(out_tex, pix, log(1.5*ret.color + 1));
    /*
    memoryBarrier();
    barrier();

    if (chunk_num == 1)
    {
        BeamReturn min_ret = get_beam_return(pix.x, pix.y, 1);
        for (int i = 1; i < GROUP_SIZE_Z; ++i)
        {
            BeamReturn cur_ret = get_beam_return(pix.x, pix.y, i);
            if (cur_ret.d < min_ret.d && cur_ret.d > 0)
            {
                min_ret = cur_ret;
            }
        }
        imageStore(out_tex, pix, min_ret.color);
    }
    */
}
