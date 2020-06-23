#include "general.h"
#include "triangle_octree.h"
#include "stream_preprocessor.h"

int main()
{
    graphics::init(500, 500);
    TriangleOctree to("octree_cube.raw", "tris_cube.raw", "norms_cube.raw", "../../cube.obj", glm::vec3(0,-2,0), 1.5);
    //TriangleOctree to2("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", -glm::vec3(-27.9545574, -0.0897386372, -19.5160427), 1.0 / 20.0);
    TriangleOctree to2("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", glm::vec3(0), 1.0 / 20.0);
    TriangleOctree to3("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", glm::vec3(0,0,-4), 1.0 / 20.0);
    auto sp = std::make_shared<StreamPreprocessor>(500,500);
    auto cam = std::make_shared<graphics::Camera>();
    cam->set_pos(glm::vec3(5));
    cam->set_forward_vector(glm::normalize(glm::vec3(0) - cam->get_pos()));
    sp->set_cam(cam->get_pos(), cam->get_pos() + cam->get_forward_vector());
    //sp->add_object(to);
    sp->add_object(to2);
    sp->add_object(to3);
    sp->refresh();
    sp->load_to_graphics();
    //std::thread(&StreamPreprocessor::refresh, &sp, 5, 1);
    
    while (1)
    {
        graphics::start_loop();
        graphics::draw_object(sp, cam);
        graphics::end_loop();
    }
    printf("TEST\n");
}
