#ifndef STREAM_PREPROCESSOR_H_
#define STREAM_PREPROCESSOR_H_

#include <algorithm>
#include <mutex>
#include <thread>
#define NOMINMAX
#include <windows.h>

#include "object.h"
#include "triangle_octree.h"
#include "ray_camera.h"

void weave_plane(std::vector<std::vector<glm::vec3>>& vertices,
    std::vector<std::vector<glm::vec3>>& in_normals,
    std::vector<std::vector<glm::vec2>>& in_light,
    std::vector<glm::vec3>& triangles,
    std::vector<glm::vec3>& normals,
    std::vector<glm::vec2>& light);

class StreamPreprocessor : public graphics::Object
{
public:
    StreamPreprocessor(int x_res, int y_res):
        _x_res(x_res),
        _y_res(y_res),
        graphics::Object(),
        _vertex_shader(std::make_shared<graphics::Shader>(GL_VERTEX_SHADER, "../shaders/stream_vertex.glsl")),
        _fragment_shader(std::make_shared<graphics::Shader>(GL_FRAGMENT_SHADER, "../shaders/stream_fragment.glsl")),
        _program(std::make_shared<graphics::Program>()),
        _ray_camera(glm::vec3(1), glm::vec3(0))
    {
        _pixel_vertices = std::vector<std::vector<glm::vec3>>(y_res, std::vector<glm::vec3>(x_res, glm::vec3(-69420)));
        _pixel_normals = std::vector<std::vector<glm::vec3>>(y_res, std::vector<glm::vec3>(x_res, glm::vec3(-69420)));
        _pixel_light = std::vector<std::vector<glm::vec2>>(y_res, std::vector<glm::vec2>(x_res, glm::vec2(1.0)));
        _program->add_shader(_vertex_shader);
        _program->add_shader(_fragment_shader);
        _program->compile_and_link();
        set_shader(_program);
    }

    void add_object(TriangleOctree& in_octree)
    {
        _octrees.push_back(in_octree);
    }

    void fire_ray(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& vertex, glm::vec3& normal)
    {
        std::vector<glm::vec3> positions(_octrees.size(), origin);
        std::vector<bool> finished(_octrees.size(), false);
        std::vector<std::unordered_set<int>> checked(_octrees.size());
        int min_i = 0;
        bool found_tri = false;
        
        while (!found_tri)
        {
            bool missed = false;
            positions[min_i] = _octrees[min_i].propogate_ray(positions[min_i], dir, found_tri, vertex, normal, checked[min_i], missed);
            float cor = glm::dot(glm::normalize(positions[min_i] - origin), glm::normalize(dir));
            if (missed)
            {
                finished[min_i] = true;
                min_i = 0;
                bool all_finished = true;
                for (int i = 0; i < _octrees.size(); ++i)
                {
                    if (!finished[i])
                    {
                        min_i = i;
                        all_finished = false;
                    }
                }
                if (all_finished)
                {
                    break;
                }
            }
            int finished_count = 0;
            for (int i = 0; i < positions.size(); ++i)
            {
                if (!finished[i])
                {
                    if (glm::length(positions[min_i] - origin) > glm::length(positions[i] - origin))
                    {
                        min_i = i;
                    }
                }
                else
                {
                    finished_count += 1;
                }
            }
            if (finished_count == _octrees.size())
            {
                break;
            }
        }
    }

    void fire_ray_chunk(int start_x, int end_x, 
        int start_y, int end_y, 
        int x_res, int y_res, 
        RayCamera cam)
    {
        for (int x = start_x; x < end_x; ++x)
        {
            for (int y = start_y; y < end_y; ++y)
            {
                float x_frac = (x+0.5) * 1.0 / x_res;
                float y_frac = (y+0.5) * 1.0 / y_res;

                glm::vec3 dir = cam.get_ray(x_frac, y_frac);
                fire_ray(cam.get_pos(), dir, _pixel_vertices[y][x], _pixel_normals[y][x]);
                //_pixel_vertices[y][x] = cam.get_pos() + dir * 5.0f;
                //_pixel_normals[y][x] = dir;

                _pixel_light[y][x] = glm::vec2(1.0) * std::max(0.0f, glm::dot(_pixel_normals[y][x], glm::normalize(glm::vec3(-1, -1, 0))));
                //_pixel_light[y][x] = glm::vec2(1.0) * glm::length(cam.get_pos() - _pixel_vertices[y][x]);
            }
        }
    }

    void refresh(int x_divs=1, int y_divs=1)
    {
        for (int i = 0; i < _y_res; ++i)
        {
            for (int j = 0; j < _x_res; ++j)
            {
                _pixel_vertices[i][j].x = -69420;
                _pixel_vertices[i][j].y = -69420;
                _pixel_vertices[i][j].z = -69420;
                _pixel_normals[i][j].x = -69420;
                _pixel_normals[i][j].y = -69420;
                _pixel_normals[i][j].z = -69420;
                _pixel_light[i][j].x = 1.0;
                _pixel_light[i][j].y = 1.0;
            }
        }
        if (1)
        {
            for (int x = 0; x < _x_res; x += _x_res / x_divs)
            {
                for (int y = 0; y < _y_res; y += _y_res / y_divs)
                {
                    _pixel_threads.push_back(
                        new std::thread(&StreamPreprocessor::fire_ray_chunk, this,
                            x, std::min({ x + _x_res / x_divs, _x_res }),
                            y, std::min({ y + _y_res / x_divs, _y_res }),
                            _x_res, _y_res,
                            _ray_camera));
                }
            }
        }
        for (auto t : _pixel_threads)
        {
            t->join();
            delete t;
        }
        while (_data_available)
        {
            Sleep(5);
        }
        _pixel_threads.clear();
        _data_mutex.lock();
        _out_vertices.clear();
        _out_normals.clear();
        _out_light.clear();
        weave_plane(_pixel_vertices, _pixel_normals, _pixel_light, 
            _out_vertices, _out_normals, _out_light);
        _data_mutex.unlock();
        _data_available = true;
    }

    void load_to_graphics()
    {
        if (_data_available)
        {
            _data_mutex.lock();
            load_vertices(_out_vertices);
            load_normals(_out_normals);
            load_uvs(_out_light);
            _data_mutex.unlock();
            _data_available = false;
        }
    }

    void set_cam(const glm::vec3& pos, const glm::vec3& lookat)
    {
        _ray_camera = RayCamera(pos, lookat);
    }

private:
    int _x_res;
    int _y_res;
    std::vector<TriangleOctree> _octrees;
    std::vector <std::vector<glm::vec3>> _pixel_vertices;
    std::vector <std::vector<glm::vec3>> _pixel_normals;
    std::vector <std::vector<glm::vec2>> _pixel_light;
    std::vector<glm::vec3> _out_vertices;
    std::vector<glm::vec3> _out_normals;
    std::vector<glm::vec2> _out_light;
    std::vector<std::thread*> _pixel_threads;
    std::mutex _data_mutex;
    std::atomic_bool _data_available = false;
    RayCamera _ray_camera;
    std::shared_ptr<graphics::Program> _program;
    std::shared_ptr<graphics::Shader> _vertex_shader;
    std::shared_ptr<graphics::Shader> _fragment_shader;
};

void weave_plane(std::vector<std::vector<glm::vec3>>& vertices,
    std::vector<std::vector<glm::vec3>>& in_normals,
    std::vector<std::vector<glm::vec2>>& in_light,
    std::vector<glm::vec3>& triangles,
    std::vector<glm::vec3>& normals,
    std::vector<glm::vec2>& light)
{
    for (int row = 0; row < vertices.size() - 1; ++row)
    {
        for (int col = 0; col < vertices[0].size() - 1; ++col)
        {
            if (col > 0 && row > 0 && col < vertices.size() - 1 && row < vertices[0].size() - 1)
            {
                if (vertices[col][row].x < -10000 &&
                    (int(vertices[col - 1][row - 1].x > -10000) +
                        int(vertices[col][row - 1].x > -10000) +
                        int(vertices[col + 1][row - 1].x > -10000) +
                        int(vertices[col + 1][row].x > -10000) +
                        int(vertices[col + 1][row + 1].x > -10000) +
                        int(vertices[col][row + 1].x > -10000) +
                        int(vertices[col - 1][row + 1].x > -10000) +
                        int(vertices[col - 1][row].x > -10000)) >= 7
                    )
                {
                    //vertices[col][row] = vertices[col - 1][row - 1];
                    //in_normals[col][row] = in_normals[col - 1][row - 1];
                    //in_light[col][row] = in_light[col - 1][row - 1];
                }
            }
        }
    }
    for (int row = 0; row < vertices.size() - 1; ++row)
    {
        for (int col = 0; col < vertices[0].size() - 1; ++col)
        {
            if (col > 0 && row > 0 && col < vertices.size() - 1 && row < vertices[0].size() - 1)
            {
                if (vertices[col + 1][row + 1].x < -10000 &&
                    vertices[col - 1][row + 1].x > -10000 &&
                    vertices[col + 1][row].x > -10000 &&
                    vertices[col][row].x > -10000)
                {
                    //vertices[col + 1][row + 1] = vertices[col][row];
                    //in_normals[col + 1][row + 1] = in_normals[col][row];
                    // in_light[col + 1][row + 1] = in_light[col][row];
                }
            }

            if (vertices[col][row].x > -10000 &&
                vertices[col + 1][row].x > -10000 &&
                vertices[col][row + 1].x > -10000)
            {
                triangles.push_back(vertices[col][row]);
                triangles.push_back(vertices[col + 1][row]);
                triangles.push_back(vertices[col][row + 1]);
                normals.push_back(in_normals[col][row]);
                normals.push_back(in_normals[col + 1][row]);
                normals.push_back(in_normals[col][row + 1]);
                light.push_back(in_light[col][row]);
                light.push_back(in_light[col + 1][row]);
                light.push_back(in_light[col][row + 1]);
            }
            if (vertices[col + 1][row].x > -10000 &&
                vertices[col + 1][row + 1].x > -10000 &&
                vertices[col][row + 1].x > -10000)
            {
                triangles.push_back(vertices[col + 1][row]);
                triangles.push_back(vertices[col + 1][row + 1]);
                triangles.push_back(vertices[col][row + 1]);
                normals.push_back(in_normals[col + 1][row]);
                normals.push_back(in_normals[col + 1][row + 1]);
                normals.push_back(in_normals[col][row + 1]);
                light.push_back(in_light[col + 1][row]);
                light.push_back(in_light[col + 1][row + 1]);
                light.push_back(in_light[col][row + 1]);
            }
        }
    }

}


#endif  // STREAM_PREPROCESSOR_H_
