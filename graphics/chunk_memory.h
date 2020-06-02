#pragma once
#ifndef CHUNK_MEMORY_H_
#define CHUNK_MEMORY_H_

#include <algorithm>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "graphics/buffer.h"
#include "graphics/chunk_index_map.h"

struct chunk_alloc
{
	glm::vec4 coord;
	int mem_index;
};

bool cmp(const chunk_alloc& a, const chunk_alloc& b)
{
	return a.mem_index < b.mem_index;
}

class ChunkBufferManager
{
public:
	ChunkBufferManager(int chunk_size_x, int chunk_size_y, int chunk_size_z,
					   int max_chunks, int map_size_x, int map_size_y, int map_size_z) :
		_chunk_buffer(std::make_shared<graphics::Buffer<GLint>>(GL_SHADER_STORAGE_BUFFER)),
		_index_buffer(std::make_shared<graphics::Buffer<chunk_alloc>>(GL_SHADER_STORAGE_BUFFER)),
		_spatial_chunk_buffer(std::make_shared<graphics::Buffer<GLint>>(GL_SHADER_STORAGE_BUFFER)),
		_chunk_size_x(chunk_size_x),
		_chunk_size_y(chunk_size_y),
		_chunk_size_z(chunk_size_z),
		_max_chunks(max_chunks),
		_chunk_block_count(chunk_size_x*_chunk_size_y*_chunk_size_z),
		_index_map(map_size_x,map_size_y,map_size_z),
		_local_index(_max_chunks, { glm::vec4(0.0),  _max_chunks + 1})  // Init all _local_index values to invalid (mem_index = -1)
	{
		_full_chunk_buffer_size = _chunk_block_count * _max_chunks; // Why do I have to initialize this here?
		_chunk_buffer->load_data(std::vector<int>(), _full_chunk_buffer_size);
		_index_buffer->load_data(_local_index);
		_spatial_chunk_buffer->load_data(_index_map._data);
		_local_index.clear();
	}

	void add_chunk(int x, int y, int z, 
		std::vector<int> blocks)
	{
		int index = get_free_index();
		if (index >= _max_chunks)
		{
			std::cout << "Chunk buffer full!" << "\n";
			return;
		}
		chunk_alloc new_alloc;
		new_alloc.coord = glm::vec4(x, y, z, 0.0);
		new_alloc.mem_index = index;
		_local_index.push_back(new_alloc);
		std::sort(_local_index.begin(), _local_index.end(), cmp);
		_index_buffer->set_data(0, _local_index);
		_chunk_buffer->set_data(_chunk_block_count * index, blocks);

		int chunk_coord_x = floor(x / _chunk_size_x);
		int chunk_coord_y = floor(y / _chunk_size_y);
		int chunk_coord_z = floor(z / _chunk_size_z);
		GLint new_index = _index_map.set(_ref_x, _ref_y, _ref_z, chunk_coord_x, chunk_coord_y, chunk_coord_z, index);
		_spatial_chunk_buffer->set_data(new_index, index);
		//std::vector<GLint> test_vec;
		//_spatial_chunk_buffer->get_data(0, 2 * 2 * 2, test_vec);
		//printf("TEST\n");
	}


	void remove_chunk(int x, int y, int z)
	{
		glm::vec4 coord(x, y, z, 0.0);
		for (int i = 0; i < _local_index.size(); ++i)
		{
			if (glm::length(_local_index[i].coord - coord) < 0.1)
			{
				_local_index.erase(_local_index.begin() + i);
				_index_buffer->set_data(0, _local_index);
				return;
			}
		}
	}

	std::shared_ptr<graphics::Buffer<int>> get_chunk_buffer()
	{
		return _chunk_buffer;
	}

	std::shared_ptr<graphics::Buffer<chunk_alloc>> get_index_buffer()
	{
		return _index_buffer;
	}

	std::shared_ptr<graphics::Buffer<int>> get_map_buffer()
	{
		return _spatial_chunk_buffer;
	}

	std::vector<int> get_chunk_size()
	{
		return { _chunk_size_x, _chunk_size_y, _chunk_size_z };
	}

	int get_max_chunks()
	{
		return _max_chunks;
	}

	void set_ref(glm::vec3 in_ref)
	{
		_ref_x = floor(in_ref.x / _chunk_size_x);
		_ref_y = floor(in_ref.y / _chunk_size_y);
		_ref_z = floor(in_ref.z / _chunk_size_z);
	}

	ChunkIndexMap _index_map;

private:

	int get_free_index()
	{
		//if (_local_index.size() > 1)
		{
			for (int i = 0; i < _local_index.size(); ++i)
			{
				if (_local_index[i].mem_index != i)
				{
					return i;
				}
			}
			if (_local_index.size() < _max_chunks)
			{
				return _local_index.size();
			}
			else
			{
				return -1;
			}
		}
	}


	int _full_chunk_buffer_size;
	int _max_chunks;
	int _chunk_size_x;
	int _chunk_size_y;
	int _chunk_size_z;
	int _ref_x;
	int _ref_y;
	int _ref_z;
	int _chunk_block_count;
	std::vector<chunk_alloc> _local_index;
	std::shared_ptr<graphics::Buffer<int>> _chunk_buffer;
	std::shared_ptr<graphics::Buffer<GLint>> _spatial_chunk_buffer;
	std::shared_ptr<graphics::Buffer<chunk_alloc>> _index_buffer;
	
};

#endif  // CHUNK_MEMORY_H_