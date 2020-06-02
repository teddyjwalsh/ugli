#pragma once
#ifndef GRAPHICS_CONTEXT_H_
#define GRAPHICS_CONTEXT_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <graphics/Camera.h>	
#include <graphics/General.h>
#include <graphics/Object.h>

namespace graphics
{

class Context
{
public:
	void remove_object(std::shared_ptr<Object> in_object)
	{
		_objects.erase(in_object);
	}

	void add_object(std::shared_ptr<Object> in_object)
	{
		_objects.insert(in_object);
	}

	void set_camera(std::shared_ptr<Camera> in_camera)
	{
		_active_camera = in_camera;
	}

	void draw_scene()
	{
		set_draw_target(nullptr);
		start_loop();
		for (auto obj : _objects)
		{
			set_program(obj->get_program()->get_program_name());
			draw_object(obj, _active_camera);
		}
		end_loop();
	}

private:
	std::shared_ptr<Camera> _active_camera;
	//std::vector<std::shared_ptr<Object>> _objects;
	std::unordered_set< std::shared_ptr<Object>> _objects;
};

}  // namespace graphics

#endif  // GRAPHICS_CONTEXT_H_