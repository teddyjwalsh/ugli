#pragma once
#ifndef CHUNK_INDEX_MAP_H_
#define CHUNK_INDEX_MAP_H_

#include <vector>
#include <glm/glm.hpp>

class ChunkIndexMap
{
public:
	ChunkIndexMap(int x_size, int y_size, int z_size):
		_x_size(x_size),
		_y_size(y_size),
		_z_size(z_size),
		_data(x_size*y_size*z_size, -1)
	{

	}

	GLint set(int ref_x, int ref_y, int ref_z, 
		int x, int y, int z, 
		int val)
	{
		int rel_x = x - ref_x;
		int rel_y = y - ref_y;
		int rel_z = z - ref_z;
		int adj_x = x;// % _x_size;
		int adj_y = y;// % _y_size;
		int adj_z = z;// % _z_size;
		int out_index = adj_x + adj_y * _x_size + adj_z * _x_size * _y_size;
		_data[out_index] = val;
		return out_index;
	}

	std::vector<GLint> _data;
private:
	int _x_size;
	int _y_size;
	int _z_size;
};

#endif  // CHUNK_INDEX_MAP_H_