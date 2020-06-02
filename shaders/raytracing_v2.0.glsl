#define GROUP_SIZE_X 32
#define GROUP_SIZE_Y 32
#define GROUP_SIZE_Z 1
#define GROUP_ARRAY_SIZE GROUP_SIZE_X*GROUP_SIZE_Y

layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;

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

struct Ray
{
	vec3 origin;
	vec3 dir;
};

struct HitInfo
{
	vec3 color;
	vec3 normal;
	vec3 loc;
	vec3 reflection;
	float distance;
	bool hit;
};

struct ChunkInfo
{
	vec3 loc;
	int index;
};

struct ChunkTraversal
{
	ChunkInfo info;
	float limit;
};

int get_cube(vec3 in_loc, ChunkInfo in_chunk)
{
}

ChunkTraversal chunk_lookup(Ray in_ray, float distance)
{
}

// in_loc is the location we want to collect light at
// in_direction is the direction of the ray that hit in_loc
// distance is the distance traversed by the ray before hitting in_loc
vec3 collect_light(vec3 in_loc, vec3 in_direction, float distance)
{
}

HitInfo intersect_boxes(vec3 origin, vec3 dir)
{
	ChunkTraversal
}

vec3 bounce_ray(Ray in_ray)
{
	vec3 out_light = default_initial_light;
	HitInfo initial_hit = intersect_boxes(in_ray);
	if (initial_hit.hit)
	{
		vec3 running_color = initial_hit.color;
		float total_distance = 0;
		out_light += running_color*collect_light(initial_hit.loc, 
											in_ray.dir, 
											total_distance);
		for (int i = 0; i < num_initial_shots_from_pixel; ++i)
		{
			current_hit = initial_hit;
			for (int j = 0; j < num_max_bounces; ++j)
			{
				current_hit = intersect_boxes(current_hit.loc,
										  current_hit.reflection);
				running_color *= current_hit.color;
				total_distance += current_hit.distance;
				light += collect_light(current_hit.loc, current_hit.reflection, total_distance);
			}
		}
	}
	else
	{
		out_light = sky_color;
	}
	return out_light;
}

void main()
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(out_tex);
    if (pix.x >= size.x || pix.y >= size.y) 
    {
        return;
    }
    vec2 pos = vec2(pix) / vec2(size.x, size.y);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);
    dir = normalize(dir);
	vec4 color = bounce_ray(eye_ray);
    imageStore(out_tex, pix, color);
}