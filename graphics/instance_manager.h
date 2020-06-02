#pragma once
#ifndef GRAPHICS_INSTANCE_MANAGER_H_
#define GRAPHICS_INSTANCE_MANAGER_H_

#include <memory>
#include <functional>

#include <glm/glm.hpp>
#include "graphics/buffer.h"

namespace graphics
{

template <class Data>
class InstanceManager
{
	InstanceManager(std::shared_ptr<Buffer<Data>> instance_buffer)
	{
	}

	void add_instance(Data in_data)
	{

	}

	void remove_instance(Data in_data)
	{

	}

	void reindex()
	{

	}
};

}  // namespace graphics

#endif  // GRAPHICS_INSTANCE_MANAGER_H_