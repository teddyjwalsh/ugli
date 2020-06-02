#ifndef GRAPHICS_GRAPHICS_BUFFER_H_
#define GRAPHICS_GRAPHICS_BUFFER_H_

#include <vector>
#include "GLFW/glfw3.h"

namespace graphics
{

const int default_extra_buffer_space = 10;

template <class T>
class Buffer {
public:
	Buffer(GLenum type=GL_ARRAY_BUFFER, bool keep_local=false) :
		_keep_local(keep_local),
		_buffer_num(-1),
		_type(type),
		_mapped_data(nullptr),
		_is_mapped(false)
	{
		glGenBuffers(1, &_buffer_num);
	}

	void load_data(std::vector<T>& data,
		int extra_buffer_space = default_extra_buffer_space,
		GLenum usage = GL_DYNAMIC_DRAW)
	{
		bind();
		_local_copy = data;
		glBufferData(_type, sizeof(T) * (data.size() + extra_buffer_space),
			NULL,
			usage);
		set_data(0, data);
		_count = data.size();
		_max_count = _count + extra_buffer_space;
	}

	void load_data(T * data,
		int extra_buffer_space = default_extra_buffer_space,
		GLenum usage = GL_DYNAMIC_DRAW)
	{
		bind();
		_local_copy = data;
		glBufferData(_type, sizeof(T) * (data.size() + extra_buffer_space),
			NULL,
			usage);
		set_data(0, data);
		_count = data.size();
		_max_count = _count + extra_buffer_space;
	}

	void add_data(T in_data, int extra_space=10)
	{
		if (_count < _max_count - 1)
		{
			set_data(_count, in_data);
			_count += 1;
		}
		else
		{
			get_local_copy();
			_local_copy.push_back(in_data);
			load_data(_local_copy, extra_space);
		}
	}

	void set_data(int offset, T data)
	{
		if (_is_mapped)
		{
			_mapped_data[offset] = data;
		}
		else
		{
			bind();
			glBufferSubData(_type, sizeof(T) * offset,
				sizeof(T),
				reinterpret_cast<GLvoid*>(&data));
		}
	}

	void set_data(int offset, std::vector<T>& data, int count=-1)
	{
		bind();
		if (count == -1)
		{
			count = data.size();
		}
		glBufferSubData(_type, sizeof(T) * offset,
			sizeof(T) * count,
			reinterpret_cast<GLvoid*>(data.data()));
	}

	void get_data(int offset, int count, std::vector<T>& data)
	{
		bind();
		T * out_data = reinterpret_cast<T*>(glMapBuffer(_type, GL_READ_WRITE));
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			std::cout << "ERRRR" << "\n";
		}
		//std::copy(out_data, out_data + count, data.end());
		data.resize(data.size() + count);
		memcpy(&data[data.size() - count], out_data + offset, count*sizeof(T));
		glUnmapBuffer(_type);
	}

	void remove_data_chunk(int offset, int count)
	{
		GLenum err;

		bind();
		T* out_data = reinterpret_cast<T*>(glMapBuffer(_type, GL_READ_WRITE));
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			std::cout << "ERRRR2" << "\n";
		}
		int copy_count = (_count - count) - (offset);
		//std::copy(out_data, out_data + count, data.end());
		memcpy(&out_data[offset], &out_data[_count - copy_count], copy_count * sizeof(T));
		_count -= count;
		if (_keep_local)
		{
			_local_copy.resize(_count);
			memcpy(_local_copy.data(), out_data, _count);
		}
		glUnmapBuffer(_type);
	}

	bool remove_data(T data_val, bool assume_single=true)
	{
		for (int i = 0; i < _local_copy.size(); ++i)
		{
			if (data_val == _local_copy[i])
			{
				remove_data_chunk(i, 1);
				std::cout << "Removing " << i << "\n";
				return true;
			}
		}
		return false;
	}

	void get_local_copy()
	{
		_local_copy.clear();
		get_data(0, _count, _local_copy);
	}

	void bind()
	{
		glBindBuffer(_type, _buffer_num);
	}

	GLuint get_buffer_name()
	{
		return _buffer_num;
	}

	// Get the number of active elements. Different
	// From max_count, which is number of allocated elements
	int count() const
	{
		return _count;
	}

	// Get the number of allocated elements
	int max_count() const
	{
		return _max_count;
	}

	T* map_memory()
	{
		bind();
		_mapped_data = reinterpret_cast<T*>(glMapBuffer(_type, GL_READ_WRITE));
		_is_mapped = true;
		return _mapped_data;
	}

	void unmap_memory()
	{
		bind();
		glUnmapBuffer(_type);
		_is_mapped = false;
		_mapped_data = nullptr;

	}

private:
	bool _keep_local;
	GLuint _buffer_num;
	int _count;
	int _max_count;
	GLenum _type;
	std::vector<T> _local_copy;
	bool _is_mapped;
	T* _mapped_data;
};

}

#endif  // GRAPHICS_GRAPHICS_BUFFER_H_

