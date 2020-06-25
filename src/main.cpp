#include <chrono>
#include "general.h"
#include "triangle_octree.h"
#include "stream_preprocessor.h"

int main()
{
    graphics::init(1000, 1000);
    //TriangleOctree to2("octree_cube.raw", "tris_cube.raw", "norms_cube.raw", "../../cube.obj", glm::vec3(0,0,0), 1.0);
    //TriangleOctree to2("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", -glm::vec3(-27.9545574, -0.0897386372, -19.5160427), 1.0 / 20.0);
    TriangleOctree to2("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", glm::vec3(0, 0, 2), 1.0 / 30);// / 15.0);
    TriangleOctree to3("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", glm::vec3(0, 0, 2), 1.0 / 30);// / 15.0);
    TriangleOctree to4("octree_rocks.raw", "tris_rocks.raw", "norms_rocks.raw", "../../rocks.obj", glm::vec3(0,0,0), 1.0/250);// / 15.0);
    //TriangleOctree to3("octree_box.raw", "tris_box.raw", "norms_box.raw", "../../wood_box.obj", glm::vec3(0,0,-4), 1.0 / 20.0);
    auto sp = std::make_shared<StreamPreprocessor>(1000,1000);
    auto cam = std::make_shared<graphics::Camera>();
    double t = 1.1;
    float omega = 0.2;
    float cam_d_init = 5.0;
    float cam_d = 2*sin(t*omega*5) + cam_d_init;
    cam->set_pos(glm::vec3(cam_d*cos(t) + 0,2, cam_d*sin(t) + 0));
    cam->set_pos(glm::vec3(1, 0, 0));
    cam->look_at(glm::vec3(0));
    sp->set_cam(cam->get_pos(), cam->get_pos() + cam->get_forward_vector()) ;
    //sp->add_object(to);
    //sp->add_object(to2);
    sp->add_object(to4);
    sp->add_object(to3);
    sp->add_object(to2);
    //sp->refresh();
    //sp->load_to_graphics();
    //std::thread(&StreamPreprocessor::refresh, &sp, 5, 1);
    std::thread th([&sp]() { while (1) { sp->refresh(); } });

    
    while (1)
    {
        cam_d = 2*sin(t*omega*5) + cam_d_init;
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        //cam->set_pos(glm::vec3(2, 2, 2));
        cam->set_pos(glm::vec3(cam_d * cos(t*omega) + 0, 2, cam_d * sin(t*omega) + 0));
        cam->look_at(glm::vec3(0));
        sp->set_cam(cam->get_pos(), cam->get_pos() + cam->get_forward_vector());
        
        graphics::start_loop();
        graphics::draw_object(sp, cam);
        
        
        graphics::end_loop();
        //if (t == 1.11)
        {

            //sp->refresh();
            sp->load_to_graphics();
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        double seconds_passed = std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count()/1000000.0;
        t += seconds_passed;
    }
    printf("TEST\n");
}
