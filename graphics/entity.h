#pragma once
#ifndef GRAPHICS_ENTITY_H_
#define GRAPHICS_ENTITY_H_

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "glm/gtx/string_cast.hpp"

namespace graphics
{

class Entity
{
public:
	Entity() :
		_pos(0.0,0.0,0.0),
		_rot(1.0),
		_scale(1.0,1.0,1.0)
	{

	}
	glm::mat4 get_transform()
	{
		glm::mat4 trans_mat = glm::translate(glm::mat4(1.0), _pos);
		glm::mat4 scale_mat = glm::scale(_scale);
		return trans_mat * _rot * scale_mat;
	}

	glm::vec3 get_pos() const
	{
		return _pos;
	}

	void set_pos(glm::vec3 in_pos)
	{
		_pos = in_pos;
	}

	void set_rot(glm::mat4 in_rot)
	{
		_rot = in_rot;
	}

	glm::mat4 get_rot() const
	{
		return _rot;
	}

	glm::vec3 get_scale() const
	{
		return _scale;
	}

protected:
	glm::vec3 _pos;
	glm::mat4 _rot;
	glm::vec3 _scale;
};

}

#endif GRAPHICS_ENTITY_H_
