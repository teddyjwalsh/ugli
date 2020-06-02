#pragma once
#ifndef GRAPHICS_RAYTRACER_H_
#define GRAPHICS_RAYTRACER_H_

#include <memory>
#include <vector>
#include <ctime>


#include <glm/glm.hpp>

#include "graphics/object.h"
#include "graphics/general.h"
#include "graphics/camera.h"
#include "graphics/loadbmp.h"
#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/buffer.h"
#include "graphics/octree.h"

#include "graphics/chunk_memory.h"


#define MAX_OCTREE_ELEMENTS 1000

namespace graphics
{

double cur_time()
{
	timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

class Raytracer
{
public:
	Raytracer(int x_chunk_size, int y_chunk_size, int z_chunk_size, int x_res=1024, int y_res=1024,
		int map_size_x=10, int map_size_y=5, int map_size_z=10,
		int low_res_div=8) :
		_low_res_div(low_res_div),
		_x_res(x_res),
		_y_res(y_res),
		_map_size_x(map_size_x),
		_map_size_y(map_size_y),
		_map_size_z(map_size_z),
		_mip_map_levels(3),
		_light_octree(),
		_chunk_buffer_manager(x_chunk_size, y_chunk_size, z_chunk_size, 
			_map_size_x*_map_size_y*_map_size_z/2, 
			_map_size_x, _map_size_y, _map_size_z),
		_cube_locs_buf(std::make_shared<graphics::Buffer<glm::vec4>>(GL_SHADER_STORAGE_BUFFER)),
		//_cube_state_buf(std::make_shared<graphics::Buffer<GLint>>(GL_SHADER_STORAGE_BUFFER)),
		_cube_state_buf(_chunk_buffer_manager.get_chunk_buffer()),
		_chunk_index_buf(_chunk_buffer_manager.get_index_buffer()),
		_chunk_map_buf(_chunk_buffer_manager.get_map_buffer()),
		_cube_colors_buf(std::make_shared<graphics::Buffer<glm::vec4>>(GL_SHADER_STORAGE_BUFFER)),
		_texture_buf(std::make_shared<graphics::Buffer<glm::vec4>>(GL_SHADER_STORAGE_BUFFER)),
		_octree_buf(_light_octree.get_buffer()),//std::make_shared<graphics::Buffer<Octree::Node>>(GL_SHADER_STORAGE_BUFFER)),
		_octree_values(std::make_shared<graphics::Buffer<glm::vec4>>(GL_SHADER_STORAGE_BUFFER)),
		_mip_map_bufs(_mip_map_levels, std::make_shared<graphics::Buffer<glm::vec4>>(GL_SHADER_STORAGE_BUFFER)),
		_focal_length(0.5),
		_cam_horiz_angle(40 * 3.14159 / 180),
		_cam_vertic_angle(40*3.14159/180), 
		_cube_count_x(100),
		_cube_count_y(100),
		_cube_count_z(100),
		_cube_locs(std::make_shared<graphics::Texture1D>(
			_cube_count_x)),
		_cube_colors(std::make_shared<graphics::Texture1D>(
			_cube_count_x)),
		_out_tex(std::make_shared<graphics::Texture2D>(x_res, y_res)),
		_out_tex_bounce_pass(std::make_shared<graphics::Texture2D>(x_res, y_res)),
		_out_tex_low_res(std::make_shared<graphics::Texture2D>(x_res/_low_res_div, y_res/_low_res_div)),
		_norm_tex(std::make_shared<graphics::Texture2D>(x_res, y_res)),
		_norm_tex_low_res(std::make_shared<graphics::Texture2D>(x_res/_low_res_div, y_res/ _low_res_div)),
		_pos_tex(std::make_shared<graphics::Texture2D>(x_res/ _low_res_div, y_res/ _low_res_div)),
		_c_shader(std::make_shared<graphics::ComputeShader>("shaders/multi_ray.glsl")),
		//_c_shader(std::make_shared<graphics::ComputeShader>("shaders/distance_split_proto.glsl")),
		_filter_shader(std::make_shared<graphics::ComputeShader>("shaders/gi_filter.glsl")),
		_v_shader(std::make_shared<graphics::ScreenVertexShader>()),
		_f_shader(std::make_shared<graphics::ScreenFragmentShader>()),
		_compute_program(std::make_shared<graphics::Program>()),
		_filter_program(std::make_shared<graphics::Program>()),
		_screen_program(std::make_shared<graphics::Program>()),
		_screen(std::make_shared<graphics::Object>())
	{
		/*
		Octree::Node proto_octree;
		proto_octree.parent = -1;
		for (int i = 0; i < 8; ++i)
		{
			proto_octree.children[i] = -1;
		}
		std::vector<Octree::Node> g_oct_buf(1, proto_octree);
		g_oct_buf[0].size = 1000;
		_octree_buf->load_data(g_oct_buf, 0);
		*/
		_compute_program->add_shader(_c_shader);
		_compute_program->compile_and_link();
		//_compute_program->bind_image_texture(_cube_locs, 0);
		_compute_program->bind_image_texture(_out_tex, 1);
		//_compute_program->bind_image_texture(_out_tex_bounce_pass, 3);
		_compute_program->bind_image_texture(_out_tex_low_res, 7);
		_compute_program->bind_image_texture(_norm_tex, 6);
		_compute_program->bind_image_texture(_norm_tex_low_res, 2);
		_compute_program->bind_image_texture(_pos_tex, 0);
		_compute_program->bind_storage_buffer(_cube_locs_buf, 0);
		//_compute_program->bind_storage_buffer(_cube_colors_buf, 3);
		_compute_program->bind_storage_buffer(_cube_state_buf, 4);
		_compute_program->bind_storage_buffer(_chunk_index_buf, 6);
		_compute_program->bind_storage_buffer(_chunk_map_buf, 3);
		_compute_program->bind_storage_buffer(_octree_buf, 7);
		_compute_program->bind_storage_buffer(_octree_values, 8);
		//_compute_program->bind_image_texture(_norm_tex, 2);
		//_compute_program->bind_image_texture(_cube_colors, 3);

		_filter_program->add_shader(_filter_shader);
		_filter_program->compile_and_link();
		//_compute_program->bind_image_texture(_cube_locs, 0);
		_filter_program->bind_image_texture(_out_tex, 1);
		_filter_program->bind_image_texture(_norm_tex, 2);
		_filter_program->bind_image_texture(_out_tex_bounce_pass, 3);

		_screen_program->add_shader(_v_shader);
		_screen_program->add_shader(_f_shader);
		_screen_program->compile_and_link();


		graphics::Mesh triangle_mesh;
		triangle_mesh.vertices.push_back(graphics::Vertex(-1.0f, -1.0f, 0.0f));
		triangle_mesh.vertices.push_back(graphics::Vertex(1.0f, -1.0f, 0.0f));
		triangle_mesh.vertices.push_back(graphics::Vertex(-1.0f, 1.0f, 0.0f));
		triangle_mesh.vertices.push_back(graphics::Vertex(1.0f, 1.0f, 0.0f));
		triangle_mesh.vertices.push_back(graphics::Vertex(-1.0f, 1.0f, 0.0f));
		triangle_mesh.vertices.push_back(graphics::Vertex(1.0f, -1.0f, 0.0f));

		_screen->load_mesh(triangle_mesh);
		_screen->set_shader(_screen_program);
		_screen_program->set_uniform_vec3("screen_size", glm::vec3(x_res, y_res, 0));

		auto sizes = _chunk_buffer_manager.get_chunk_size();
		_compute_program->set_uniform_int("chunk_count", _chunk_buffer_manager.get_max_chunks());
		_compute_program->set_uniform_int("chunk_size_x", sizes[0]);
		_compute_program->set_uniform_int("chunk_size_y", sizes[1]);
		_compute_program->set_uniform_int("chunk_size_z", sizes[2]);
		_compute_program->set_uniform_int("chunk_map_size_x", _map_size_x);
		_compute_program->set_uniform_int("chunk_map_size_y", _map_size_y);
		_compute_program->set_uniform_int("chunk_map_size_z", _map_size_z);

	}

	void set_cubes(std::vector<glm::vec4> in_cube_locs, std::vector<glm::vec4> in_cube_colors)
	{
		_cube_count_x = in_cube_locs.size();
		_cube_locs = std::make_shared<graphics::Texture1D>(
			in_cube_locs.size());
		_cube_colors = std::make_shared<graphics::Texture1D>(
			in_cube_locs.size());

		_compute_program->bind_image_texture(_cube_locs, 0);
		_compute_program->bind_image_texture(_out_tex, 1);
		_compute_program->bind_image_texture(_out_tex_low_res, 2);
		_compute_program->bind_image_texture(_cube_colors, 3);

		_filter_program->bind_image_texture(_out_tex, 1);
		_filter_program->bind_image_texture(_norm_tex, 2);

		_cube_locs->set_data(0, 
			_cube_count_x, 
			in_cube_locs);

	}

	void set_cubes_buf(std::vector<glm::vec4> in_cube_locs, std::vector<glm::vec4> in_cube_colors)
	{
		_cube_count_x = in_cube_locs.size();

		_cube_locs_buf->load_data(in_cube_locs,
			0);
		_cube_colors_buf->load_data(in_cube_colors,
			0);

		_compute_program->set_uniform_int("cube_count", _cube_count_x);
	}

	void set_cube_states(std::vector<GLint> in_cube_states)
	{
		//_cube_count_x = in_cube_states.size();
		_cube_count_x = int(std::cbrt(in_cube_states.size())) + 0;
		_cube_count_y = _cube_count_x;
		_cube_count_z = _cube_count_x;
		_cube_state_buf->load_data(in_cube_states,
			0);

		_compute_program->set_uniform_int("cube_count_x", _cube_count_x);
		_compute_program->set_uniform_int("cube_count_y", _cube_count_y);
		_compute_program->set_uniform_int("cube_count_z", _cube_count_z);
	}

	void set_textures(std::vector<std::string> in_image_filenames)
	{
		unsigned char* pixels = NULL;
		unsigned int width, height;
		std::vector <glm::vec4> pixel_vec;
		std::vector<std::vector <glm::vec4>> mip_map_vectors(_mip_map_levels);
		for (auto f : in_image_filenames)
		{
			std::vector<glm::vec4> prev_level;
			unsigned int err = loadbmp_decode_file(f.c_str(), &pixels, &width, &height, LOADBMP_RGBA);
			for (int i = 0; i < 4*width * height; i+=4)
			{
				pixel_vec.push_back(glm::vec4(float(pixels[i] / 256.0),
					float(pixels[i + 1]/256.0),
					float(pixels[i + 2] / 256.0),
					float(pixels[i + 3] / 256.0)));
				prev_level.push_back(glm::vec4(float(pixels[i] / 256.0),
					float(pixels[i + 1] / 256.0),
					float(pixels[i + 2] / 256.0),
					float(pixels[i + 3] / 256.0)));
			}

			// Mip map calculation. Four pixel average.
			int prev_width = width;
			int prev_height = height;
			int next_width = width;
			int next_height = height;
			for (int i = 0; i < _mip_map_levels; ++i)
			{
				prev_width = next_width;
				prev_height = next_height;
				next_width = next_width / 2;
				next_height = next_height / 2;

				std::vector<glm::vec4> next_level(next_width * next_height);

				for (int k = 0; k < next_width; ++k)
				{
					for (int n = 0; n < next_height; ++n)
					{
						glm::vec4 pix1 = prev_level[(k * 2) + prev_width * (n * 2)];
						glm::vec4 pix2 = prev_level[(k * 2 + 1) + prev_width * (n * 2)];
						glm::vec4 pix3 = prev_level[(k * 2) + prev_width * (n * 2 + 1)];
						glm::vec4 pix4 = prev_level[(k * 2 + 1) + prev_width * (n * 2 + 1)];
						next_level[k + next_width * n] = (pix1 + pix2 + pix3 + pix4) / 4.0f;
					}
				}
				mip_map_vectors[i].insert(mip_map_vectors[i].end(), next_level.begin(), next_level.end());
				prev_level = next_level;
			}
			//pixel_vec.insert(pixel_vec.end(), pixels, pixels + width * height);
			free(pixels);
		}

		_texture_buf->load_data(pixel_vec);
		_compute_program->bind_storage_buffer(_texture_buf, 5);
		for (int i = 0; i < _mip_map_levels; ++i)
		{
			_mip_map_bufs[i]->load_data(mip_map_vectors[i]);
			_compute_program->bind_storage_buffer(_mip_map_bufs[i], 20+i);
		}
		_compute_program->set_uniform_int("tex_width", width);
		_compute_program->set_uniform_int("tex_height", height);
		_compute_program->set_uniform_int("tex_count", in_image_filenames.size());
	}

	void draw(int x_width, int y_width, int bounces, bool include_first_bounce=true, bool filter=false)
	{
		_compute_program->set_uniform_int("max_bounces", bounces);
		_compute_program->set_uniform_int("include_first_bounce", int(include_first_bounce));
		_compute_program->run_compute_program(x_width, y_width);
		//glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT || GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		if (filter)
		{
			//_filter_program->run_compute_program(x_width, y_width);
		}
		//glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT || GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glFinish();

		graphics::set_draw_target(nullptr);
		graphics::start_loop();
		//smooth_light();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _out_tex->get_texture_name());
		//get_data();
		graphics::draw_object(_screen, _camera);
		graphics::end_loop();
	}

	void get_data()
	{
		_cube_locs->get_data(_out_vec1);
		_out_tex->get_data(_out_vec2);
		printf("Got data\n");
	}

	glm::vec4 tex_index(std::vector<glm::vec4>& in_vec, int x, int y, int x_size, int y_size)
	{
		return in_vec[x_size * y + x];
	}

	void set_tex_index(std::vector<glm::vec4>& in_vec, glm::vec4 in_val, int x, int y, int x_size, int y_size)
	{
		in_vec[x_size * y + x] = in_val;
	}

	void smooth_light()
	{
		_out_tex->get_data(_out_vec1);
		_norm_tex->get_data(_out_vec2);

		int radius = 2;

		for (int x_offset = -radius; x_offset < radius + 1; ++x_offset)
		{
			for (int y_offset = -radius; y_offset < radius + 1; ++y_offset)
			{
				for (int i = std::abs(x_offset); i < 1024 - std::abs(x_offset); ++i)
				{
					for (int j = std::abs(y_offset); j < 1024 - std::abs(y_offset); ++j)
					{
						glm::vec4 v1 = tex_index(_out_vec1, i, j, 1024, 1024);
						glm::vec4 v2 = tex_index(_out_vec1, i + x_offset, j + y_offset, 1024, 1024);
						glm::vec4 n1 = tex_index(_out_vec2, i, j, 1024, 1024);
						glm::vec4 n2 = tex_index(_out_vec2, i + x_offset, j + y_offset, 1024, 1024);

						if (glm::dot(glm::vec3(n1), glm::vec3(n2)) > 0.1 && std::abs(n1.w - n2.w) < 0.5)
						{
							//std::cout << glm::length(n1) << ", " << glm::length(n2) << "\n";
							set_tex_index(_out_vec1, (v1 + v2) / 2.0f, i, j, 1024, 1024);
							set_tex_index(_out_vec1, (v1 + v2) / 2.0f, i + x_offset, j + y_offset, 1024, 1024);
						}
						else
						{
						}
					}
				}
			}
		}
		_out_tex->set_data(0,0,1024,1024,_out_vec1);
	}

#ifndef _SHARED_CAM
	void set_camera(graphics::Camera* in_cam)
#else
	void set_camera(std::shared_ptr<graphics::Camera> in_cam)
#endif
	{
		_camera = in_cam;
		_cam_pos = in_cam->get_pos();
		float cam_width = _focal_length * tan(_cam_horiz_angle);
		float cam_height = _focal_length * tan(_cam_vertic_angle);
		glm::vec3 center = _cam_pos + in_cam->get_forward_vector()*_focal_length;
		_cam_12 = center - in_cam->get_right_vector()*cam_width + 
			in_cam->get_up_vector() * cam_height;
		_cam_22 = center + in_cam->get_right_vector() * cam_width +
			in_cam->get_up_vector() * cam_height;
		_cam_11 = center - in_cam->get_right_vector() * cam_width -
			in_cam->get_up_vector() * cam_height;
		_cam_21 = center + in_cam->get_right_vector() * cam_width -
			in_cam->get_up_vector() * cam_height;
		_compute_program->set_uniform_vec3("ray00", _cam_11 - _cam_pos);
		_compute_program->set_uniform_vec3("ray01", _cam_12 - _cam_pos);
		_compute_program->set_uniform_vec3("ray10", _cam_21 - _cam_pos);
		_compute_program->set_uniform_vec3("ray11", _cam_22 - _cam_pos);
		_compute_program->set_uniform_vec3("eye", _cam_pos);

		_compute_program->set_uniform_double("ttime", cur_time());
		
		//refresh_octree(_cam_pos);
		//_octree_buf->load_data(_light_octree._oct_buf);
	}

	void refresh_octree(glm::vec3 cam_pos)
	{
		_light_octree.map(true);
		static int count = 0;
		count += 1;
		if (count % 5 == 0)
		{
			_light_octree.cleanup(cam_pos, 50);
			//_light_octree.reset();
			count = 0;
		}
		else
		{
			
		}
		
		std::vector<Octree> g_oct_buf;
		std::vector<glm::vec4> tex_data;
		_pos_tex->get_data(tex_data);
		for (int i = 0; i < _y_res/_low_res_div; ++i)
		{
			for (int j = 0; j < _x_res/_low_res_div; ++j)
			{
				glm::vec3 val = glm::vec3(tex_data[j + (_x_res/ _low_res_div) * i]);
				float dist = glm::length(cam_pos - val);
				int oct_depth = 1;
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
				_light_octree.lookup(oct_depth, val, true);
			}
		}
		_light_octree.unmap(true);
	}

	void add_chunk(int x, int y, int z, std::vector<int> blocks)
	{
		_chunk_buffer_manager.add_chunk(x, y, z, blocks);
	}

	void set_ref(glm::vec3 in_vec)
	{
		_chunk_buffer_manager.set_ref(in_vec);
	}

private:
	int _cube_count_x;
	int _cube_count_y;
	int _cube_count_z;
	int _map_size_x;
	int _map_size_y;
	int _map_size_z;
	int _mip_map_levels;
	int _x_res;
	int _y_res;
	int _low_res_div;

	MappedOctree _light_octree;
	ChunkBufferManager _chunk_buffer_manager;
	std::shared_ptr<graphics::Buffer<chunk_alloc>> _chunk_index_buf;
	std::shared_ptr<graphics::Buffer<GLint>> _chunk_map_buf;
	std::shared_ptr<graphics::Buffer<glm::vec4>> _cube_locs_buf;
	std::shared_ptr<graphics::Buffer<GLint>> _cube_state_buf;
	std::shared_ptr<graphics::Buffer<glm::vec4>> _cube_colors_buf;
	std::shared_ptr<graphics::Buffer<glm::vec4>> _texture_buf;
	std::shared_ptr<graphics::Buffer<MappedOctree::Node>> _octree_buf;
	std::shared_ptr<graphics::Buffer<glm::vec4>> _octree_values;
	std::vector<std::shared_ptr<graphics::Buffer<glm::vec4>>> _mip_map_bufs;
	std::shared_ptr<graphics::Texture1D> _cube_locs;
	std::shared_ptr<graphics::Texture1D> _cube_colors;
	std::shared_ptr<graphics::Texture2D> _out_tex;
	std::shared_ptr<graphics::Texture2D> _out_tex_bounce_pass;
	std::shared_ptr<graphics::Texture2D> _out_tex_low_res;
	std::shared_ptr<graphics::Texture2D> _norm_tex;
	std::shared_ptr<graphics::Texture2D> _norm_tex_low_res;
	std::shared_ptr<graphics::Texture2D> _pos_tex;
	std::shared_ptr<graphics::Shader> _c_shader;
	std::shared_ptr<graphics::Shader> _filter_shader;
	std::shared_ptr<graphics::ScreenVertexShader> _v_shader;
	std::shared_ptr<graphics::ScreenFragmentShader> _f_shader;
	std::shared_ptr<graphics::Object> _screen;
	std::shared_ptr<graphics::Program> _compute_program;
	std::shared_ptr<graphics::Program> _filter_program;
	std::shared_ptr<graphics::Program> _screen_program;
	std::vector<glm::vec4> _out_vec1;
	std::vector<glm::vec4> _out_vec2;
#ifdef _SHARED_CAM
	std::shared_ptr<graphics::Camera> _camera;
#else
    graphics::Camera * _camera;
#endif
	float _focal_length;
	float _cam_horiz_angle;
	float _cam_vertic_angle;
	glm::vec3 _cam_pos;
	glm::vec3 _cam_11;
	glm::vec3 _cam_12;
	glm::vec3 _cam_21;
	glm::vec3 _cam_22;
};

}  // namespace graphics

#endif  // GRAPHICS_RAYTRACER_H_
