#version 460 core

#define GROUP_SIZE_X 8
#define GROUP_SIZE_Y 8
#define GROUP_SIZE_Z 1


#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI 0.31830988618379067153776752674503

layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;

layout(binding = 1, rgba32f) uniform image2D color_tex;
layout(binding = 2, rgba32f) uniform image2D norm_tex;
layout(binding = 3, rgba32f) uniform image2D bounce_pass;

int offset_radius = 3;

vec4 smartDeNoise(ivec2 pix, vec4 centrPx)
{
	vec4 n1 = imageLoad(norm_tex, pix);
	const float trad = centrPx.w;
	const float radius = clamp(int(20*(100 - n1.w)/100.0),0,20);
	float radQ = radius * radius;
	vec4 out_px = vec4(0);
	float factor = 0;
	int count = 0;
	for (float x = -radius; x <= radius; x+=2) {
		float pt = sqrt(radQ - x * x);  // pt = yRadius: have circular trend
		for (float y = -pt; y <= pt; y+=2) {
			vec2 d = vec2(x, y);
			vec4 walk_norm = imageLoad(norm_tex, pix+ivec2(x, y));
			vec4 walk_px = imageLoad(bounce_pass, pix + ivec2(x,y));
			out_px += walk_px * max(dot(walk_norm.xyz, n1.xyz), 0)*clamp((5 - abs(centrPx.w - walk_px.w))/5.0, 0, 1);
			factor += max(dot(walk_norm.xyz, n1.xyz), 0) * clamp((5 - abs(centrPx.w - walk_px.w)) / 5.0, 0, 1);
			count += 1;
		}
	}
	return out_px / factor;
}

float random(vec2 st) {
	return fract(sin(dot(st.xy,
		vec2(12.9898, 78.233))) *
		43758.5453123);
}

void main()
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 image_size = imageSize(color_tex);
	vec4 bounce = imageLoad(bounce_pass, pix);
	vec4 base = imageLoad(color_tex, pix);

	vec4 val = smartDeNoise(pix, bounce);
	imageStore(color_tex, pix, base + val);
}