#include <windows.h>

#include "raytracer.h"
#include "general.h"
#include "text.h"

int main()
{
    graphics::init(500,500);
    auto to = std::make_shared<graphics::TextObject>("CChHCunks: 20", 50, "roboto");
    
    while (1)
    {
        graphics::start_loop();
        graphics::draw_object(to);
        graphics::end_loop();
        //Sleep(10);
    }
}
