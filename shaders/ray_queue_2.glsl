#pragma once

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
			csum += dot(in_direction, dir) * (lamps[i].color) * (lamps[i].intensity) / float(d * d);
		}
	}
	return csum;
}

// "hit" defined as a change in refractive index
// Assuming one initial ray for now
vec4 trace(vec3 origin, vec3 dir)
{
	// Final output light. Ray results are added to this
	vec4 out_light = vec4(0.0, 0.0, 0.0, 1.0);

	// Set up and shoot first ray. This should never have to be repeated.
	int initial_ray = add_ray();
	HitInfo initial_hit;

	ray_buffer.rays[initial_ray].origin = origin;
	ray_buffer.rays[initial_ray].dir = dir;
	ray_buffer.rays[initial_ray].color = vec4(1.0, 1.0, 1.0, 1.0);

	// Loop through the max number of "bounces".
	// Changing medium counts as a bounce. 
	for (int i = 0; i < MAX_BOUNCES; ++i)
	{
		// Process entirety of current ray buffer
		int to_process_count = get_ray_count();
		for (int j = 0; j < to_process_count; ++j)
		{
			int next_ray = proc_next_ray();
			intersect_boxes(initial_ray, hit);
			if (hit.hit)
			{
				// Add reflection ray to queue
				int reflection = add_ray();
				ray_buffer.rays[reflection].origin = hit.position;
				ray_buffer.rays[reflection].dir = dir;
				ray_buffer.rays[reflection].color = hit.reflect_color * (ray_buffer.rays[initial_ray].color / (hit.distance * hit.distance));
				out_light += collect_light(ray_buffer.rays[reflection].origin, hit.normal, hit.distance);
				if (hit.refract_color.w > 0.01)
				{
					// If material is transparent add refraction ray
					int refraction = add_ray();
					ray_buffer.rays[refraction].origin = hit.position + 2 * hit.incident * ray_margin;
					ray_buffer.rays[refraction].dir = hit.incident; // for now, refraction direction = incident direction
					ray_buffer.rays[refraction].color = hit.refract_color * (ray_buffer.rays[initial_ray].color / (hit.distance * hit.distance));
					out_light += collect_light(ray_buffer.rays[refraction].origin, ray_buffer.rays[refraction].dir, hit.distance);
				}
			}
			pop_front();
		}
	}
	return out_light;
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