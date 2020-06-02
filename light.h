#ifndef GRAPHICS_LIGHT_H_
#define GRAPHICS_LIGHT_H_

#include "graphics/entity.h"

namespace graphics
{

class Light : public Entity
{

private:
    double _intensity;
};

class Sun : public Light
{
};
 
}

#endif  // GRAPHICS_LIGHT_H_
