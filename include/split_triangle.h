#ifndef SPLIT_TRIANGLE_H_
#define SPLIT_TRIANGLE_H_

#include "glm/glm.hpp"
#include <vector>
#include <array>

#include "triangle.h"

void print_vertex(const glm::vec3& v)
{
    printf("[%f, %f, %f]", v.x, v.y, v.z);
}

void print_triangle(const Triangle& tri, const int indent = 0)
{
    for (auto& v : tri)
    {
        for (int i = 0; i < indent; ++i)
        {
            printf("\t");
        }
        print_vertex(v);
        printf("\n");
    }
}

void split_triangle_on_plane(Triangle& tri, std::vector<Triangle>& out_triangles, glm::vec3& n, glm::vec3& p)
{
    const float epsilon = 0.00000001;
    glm::vec3 v1 = tri[2] - tri[1];
    glm::vec3 vo = tri[0] - tri[1];
    if (std::abs(glm::dot(v1, n)) < epsilon && std::abs(glm::dot(vo,n)) < epsilon)
    {
        //triangle is parallel to plane
        out_triangles.push_back(tri);
        return;
    }
    float t1 = (glm::dot(n, p - tri[1]) / glm::dot(n, v1));
    if (t1 < 1.0 && t1 > 0.0)
    {
        // vertices 2 and 1 are on different sides of plane
        glm::vec3 v2 = tri[0] - tri[1];
        float t2 = (glm::dot(n, p - tri[1]) / glm::dot(n, v2));
        if (t2 < 1.0 && t2 > 0.0)
        {
            // tri[1] is separated from others by plane
            if (std::abs(glm::dot(n,p-tri[1])) > epsilon)
            {
                out_triangles.push_back({tri[1], 
                        tri[1] + v2*(1.00f*t2),
                        tri[1] + v1*(1.00f*t1)});
            }
            out_triangles.push_back({tri[0], 
                    tri[2],
                    tri[1] + v1*(1.00f*t1)});
            if (std::abs(glm::dot(n,p-tri[0])) > epsilon)
            {
                out_triangles.push_back({tri[0], 
                        tri[1] + v2*(1.00f*t2),
                        tri[1] + v1*(1.00f*t1)});
            }
        }
        else
        {
            // tri[2] is separated from others by plane
            glm::vec3 v3 = tri[2] - tri[0];
            float t3 = (glm::dot(n, p - tri[0]) / glm::dot(n, v3));
            if (std::abs(glm::dot(n,p-tri[2])) > epsilon)
            {
                out_triangles.push_back({tri[2], 
                        tri[0] + v3*(1.00f*t3),
                        tri[1] + v1*(1.00f*t1)});
            }
            out_triangles.push_back({tri[0], 
                    tri[1],
                    tri[1] + v1*(1.00f*t1)});
            if (std::abs(glm::dot(n,p-tri[0])) > epsilon)
            {
                out_triangles.push_back({tri[0], 
                        tri[0] + v3*(1.00f*t3),
                        tri[1] + v1*(1.00f*t1)});
            }
        }
    }
    else
    {
        // Vertices 2 and 1 are on same side of plane
        glm::vec3 v4 = tri[2] - tri[0];
        float t4 = (glm::dot(n, p - tri[0]) / glm::dot(n, v4));
        glm::vec3 v6 = tri[1] - tri[0];
        float t6 = (glm::dot(n, p - tri[0]) / glm::dot(n, v6));
        if ((t4 < 1.0 && t4 > 0.0) || (t6 < 1.0 && t6 > 0.0))
        {
            // vertex 0 is on opposite side of 2 and 1
            glm::vec3 v5 = tri[1] - tri[0];
            float t5 = (glm::dot(n, p - tri[0]) / glm::dot(n, v5));
            if (std::abs(glm::dot(n,p-tri[0])) > epsilon)
            {
                out_triangles.push_back({tri[0], 
                        tri[0] + v5*(1.00f*t5),
                        tri[0] + v4*(1.00f*t4)});
            }
            out_triangles.push_back({tri[1], 
                    tri[2],
                    tri[0] + v4*(1.00f*t4)});
            if (std::abs(glm::dot(n,p-tri[1])) > epsilon)
            {
                out_triangles.push_back({tri[1], 
                        tri[0] + v5*(1.00f*t5),
                        tri[0] + v4*(1.00f*t4)});
            }
        }
        else
        {
            // All vertices on same side
            out_triangles.push_back(tri);
        }
    }
}

void split_triangle(Triangle& tri, std::vector<Triangle>& out_triangles, glm::vec3& loc, float size)
{
    glm::vec3 node_center = loc + glm::vec3(size/2.0);
    glm::vec3 n(0,1,0);
    split_triangle_on_plane(tri, out_triangles, n, node_center);
    std::vector<Triangle> temp_tris;
    n = glm::vec3(-1,0,0);
    for (auto& t : out_triangles)
    {
        split_triangle_on_plane(t, temp_tris, n, node_center); 
    }
    out_triangles = temp_tris;
    n = glm::vec3(0,0,1);
    temp_tris.clear();
    for (auto& t : out_triangles)
    {
        split_triangle_on_plane(t, temp_tris, n, node_center); 
    }
    out_triangles = temp_tris;
}

#endif  // SPLIT_TRIANGLE_H_
