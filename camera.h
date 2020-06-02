#pragma once
#ifndef GRAPHICS_CAMERA_H_
#define GRAPHICS_CAMERA_H_

#include "glm/glm.hpp"

#include "graphics/entity.h"

namespace graphics
{

class Camera : public Entity
{
public:
	Camera():
		_up_vector(0, 1.0, 0)
	{
		set_pos(glm::vec3(30.0, 10.0, 350.0));
		_forward_vector = glm::normalize(glm::vec3(0.0, 0.0, 0.0) - get_pos());
		_right_vector = glm::normalize(glm::cross(_forward_vector, _up_vector));
		_up_vector = glm::cross(_right_vector, _forward_vector);
	}

	glm::mat4 get_view() const
	{
		glm::mat4 view_mat = glm::lookAt(get_pos(), get_pos() + _forward_vector, _up_vector);
		return view_mat;
	}

	glm::mat4 get_projection() const
	{
		glm::mat4 projection_matrix = glm::perspective(
			glm::radians(90.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
			4.0f / 3.0f,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
			0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
			100.0f             // Far clipping plane. Keep as little as possible.
		);
		return projection_matrix;
	}

	glm::vec3 get_right_vector() const
	{
		return _right_vector;
	}

	glm::vec3 get_up_vector() const
	{
		return _up_vector;
	}

	glm::vec3 get_forward_vector() const
	{
		return _forward_vector;
	}

	void set_forward_vector(glm::vec3 forward_vec)
	{
		_forward_vector = forward_vec;
		_up_vector = glm::vec3(0.0, 1.0, 0.0);
		_right_vector = glm::normalize(glm::cross(_forward_vector, _up_vector));
		_up_vector = glm::cross(_right_vector, _forward_vector);
		//look_at(_forward_vector);
	}

	void look_at(glm::vec3 look_vec)
	{
		_up_vector = glm::vec3(0.0, 1.0, 0.0);
		_forward_vector = glm::normalize(look_vec - get_pos());
		_right_vector = glm::normalize(glm::cross(_forward_vector, _up_vector));
		_up_vector = glm::cross(_right_vector, _forward_vector);
	}

private:
	glm::vec3 _up_vector;
	glm::vec3 _forward_vector;
	glm::vec3 _right_vector;
};

}

#endif  // GRAPHICS_CAMERA_H_