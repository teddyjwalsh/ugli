#ifndef TRIANGLE_OCTREE_H_
#define TRIANGLE_OCTREE_H_

#include <algorithm>
#include <array>
#include <unordered_set>
#include "glm/glm.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include "tiny_obj_loader.h"

#include "triangle.h"
#include "split_triangle.h"

#define MAX_TRIS 20

bool RayIntersectsTriangle(const glm::vec3& rayOrigin,
                           const glm::vec3& rayVector,
                           Triangle& inTriangle,
                           glm::vec3& outIntersectionPoint)
{
    const float EPSILON = 0.00000001;
    glm::vec3& vertex0 = inTriangle[0];
    glm::vec3& vertex1 = inTriangle[1];
    glm::vec3& vertex2 = inTriangle[2];
    glm::vec3 edge1, edge2, h, s, q;
    float a,f,u,v;
    edge1 = (vertex1 - vertex0)*1.0f;
    edge2 = (vertex2 - vertex0)*1.0f;
    //normal = glm::normalize(glm::cross(edge1, edge2));
    h = glm::cross(rayVector,edge2);
    a = glm::dot(edge1,h);
    if (a > -EPSILON && a < EPSILON)
    {
        //print_vertex(rayOrigin); printf("\n");
        //print_vertex(rayVector); printf("\n");
        //printf("fail 1 %f\n", a);
        return false;    // This ray is parallel to this triangle.
    }
    f = 1.0/a;
    s = rayOrigin - vertex0;
    u = f * glm::dot(s,h);
    float margin = 0.01;
    if (u < 0.0 - margin || u > 1.0 + margin)
    {
        //print_vertex(rayOrigin); printf("\n");
        //print_vertex(rayVector); printf("\n");
        //printf("fail 2 %f\n", u);
        return false;
    }
    q = glm::cross(s,edge1);
    v = f * glm::dot(rayVector,q);
    if (v < 0.0 - margin || u + v > 1.0 + margin)
    {
        //print_vertex(rayOrigin); printf("\n");
        //print_vertex(rayVector); printf("\n");
        //printf("fail 3 %f %f\n", u, v);
        return false;
    }
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * glm::dot(edge2,q);
    //if (t > EPSILON) // ray intersection
    {
        outIntersectionPoint = rayOrigin + rayVector * t;
        return true;
    }
    //else // This means that there is a line intersection but not a ray intersection.
    //    return false;
}

template <class T>
class PartitionedPointer
{
public:
    PartitionedPointer(int partition, T* p1, T* p2) :
        _p1(p1),
        _p2(p2),
        _partition(partition)
    {}

    T& operator [](int i)
    {
        if (i < _partition)
        {
            return _p1[i];
        }
        else
        {
            return _p2[i];
        }
    }

private:
    int _partition;
    T* _p1;
    T* _p2;
};

class TriangleOctree
{
public:

    typedef int32_t NodeHandle;

    struct Node
    {
        static const NodeHandle invalid_handle = -1;
        //std::array<Triangle, MAX_TRIS> tris = {
        //    {glm::vec3(0), glm::vec3(0), glm::vec3(0)}
        //};
        std::array<int, MAX_TRIS> tris;
        int tri_count = 0;
        std::array<NodeHandle,8> children = {-1,-1,-1,-1,-1,-1,-1,-1};
        //NodeHandle parent = -1;
        float size = -1;
        glm::vec3 loc = glm::vec3(-1);
        //bool contains = false;
        int checked = 0;
    };

    TriangleOctree(Node * data, 
        Triangle * tri_data,
        glm::vec3 * norm_data,
        size_t length):
        _data(data),
        _tri_data(tri_data),
        _norm_data(norm_data),
        _length(length)
    {
        _proto_node.size = 10;
        _proto_node.loc = glm::vec3(0);
        _data[_root] = _proto_node;
        _last_tri = 0;
        _from_file = false;
    }

    TriangleOctree(std::string octree_filepath, 
        std::string tri_filepath,
        std::string norm_filepath,
        std::string obj_filepath,
        size_t length):
            _length(length),
            _octree_filepath(octree_filepath),
            _tri_filepath(octree_filepath),
            _obj_filepath(obj_filepath),
            _norm_filepath(octree_filepath)
    {
        _proto_node.size = 10;
        _proto_node.loc = glm::vec3(0);
        _data[_root] = _proto_node;
        _last_tri = 0;
    }

    void set_data_from_files()
    {
        _octree_file.open(_octree_filepath);
        _tri_file.open(_octree_filepath);
        _norm_file.open(_norm_filepath);
        _data = reinterpret_cast<Node*>(_octree_file.data());
        _tri_data = reinterpret_cast<Triangle*>(_tri_file.data());
        _norm_data = reinterpret_cast<glm::vec3*>(_norm_file.data());
        _from_file = true;
    }

    void load_model()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        Triangle test_tri = {glm::vec3(110, 112, 100), glm::vec3(112,110,100), glm::vec3(110,112,102)};
        std::string err;
        std::string warn;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _obj_filepath.c_str());
        printf(err.c_str());

        if (_octree_file.is_open())
        {
            // Loop over shapes
            for (size_t s = 0; s < shapes.size(); s++) {
              // Loop over faces(polygon)
              size_t index_offset = 0;
              
              for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
              {
                int fv = shapes[s].mesh.num_face_vertices[f];
                glm::vec3 norm;
                glm::vec3 center(0);
                // Loop over vertices in the face.
                if (f % 10000 == 0)
                {
                    printf("face %d\n", f);
                }
                float scale = 1.0/100.0;///15.0;
                for (size_t v = 0; v < fv; v++) {
                  // access to vertex
                  tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                  tinyobj::real_t vx = attrib.vertices[3*idx.vertex_index+0];
                  tinyobj::real_t vy = attrib.vertices[3*idx.vertex_index+1];
                  tinyobj::real_t vz = attrib.vertices[3*idx.vertex_index+2];
                  //norm.x = attrib.normals[3 * idx.normal_index + 0];
                  //norm.y = attrib.normals[3 * idx.normal_index + 1];
                  //norm.z = attrib.normals[3 * idx.normal_index + 2];
                  
                  //test_tri[v][0] = scale * vx + 2.0;
                  test_tri[v][0] = scale*vx + 5.0;
                  test_tri[v][1] = scale*vy + 5.0;
                  test_tri[v][2] = scale*vz + 5.0;
                  //test_tri[v][2] = scale*vz + 9.0;
                }
                norm = glm::normalize(glm::cross(test_tri[1] - test_tri[0], test_tri[0] - test_tri[2]));
                add_triangle(test_tri, norm);
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
              }
            }
            printf("model tree created\n");
        }
    }

    float largest_side(Triangle& tri)
    {
        return std::max({ glm::length(tri[2] - tri[0]),
            glm::length(tri[1] - tri[0]),
            glm::length(tri[2] - tri[1]) });
    }

    glm::vec2 intersect_box(glm::vec3 origin, glm::vec3 dir, 
                            const glm::vec3 box_origin, float box_size)  const
    {
        const glm::vec3& b_min = box_origin;
        glm::vec3 b_max = box_origin + glm::vec3(box_size);
        glm::vec3 inv = 1.0f / dir;
        glm::vec3 tMin = (b_min - origin) * inv;
        glm::vec3 tMax = (b_max - origin) * inv;
        glm::vec3 t1 = glm::min(tMin, tMax);
        glm::vec3 t2 = glm::max(tMin, tMax);
        float tNear = std::max(std::max(t1.x, t1.y), t1.z);
        float tFar = std::min(std::min(t2.x, t2.y), t2.z);
        return glm::vec2(tNear,tFar);
    }

    NodeHandle add_node(glm::vec3 loc, float size, NodeHandle parent)
    {
        _last_node += 1;
        _proto_node.size = size;
        _proto_node.loc = loc;
        //_proto_node.parent = parent;
        if (_from_file)
        {
            if (_octree_file.max_length < sizeof(Node) * _last_node)
            {
                _octree_file.resize(_octree_file.max_length + 100*sizeof(Node));
            }
        }
        _data[_last_node] = _proto_node;
        return _last_node;
    }

    int child_index_of_point(glm::vec3& coord, glm::vec3& node_loc, float size, glm::vec3& child_pos) const
    {
        glm::vec3 node_center = node_loc + glm::vec3(size/2.0);
        
        if (coord.x - node_loc.x < 0 || coord.x - node_loc.x > size ||
            coord.y - node_loc.y < 0 || coord.y - node_loc.y > size ||
            coord.z - node_loc.z < 0 || coord.z - node_loc.z > size)
        {
            return -1;
        }
        
        int x_index = int(coord.x > node_center.x);
        int y_index = int(coord.y > node_center.y);
        int z_index = int(coord.z > node_center.z);
        int child_index = x_index + y_index * 2 + z_index * 4;
        child_pos = node_loc + glm::vec3(x_index, y_index, z_index) *
                size / 2.0f;
        return child_index;
    }

    int child_index_of_triangle(Triangle& tri, glm::vec3 node_loc, float size, glm::vec3& child_pos)
    {
        glm::vec3 edge1 = tri[1] - tri[0];
        glm::vec3 edge2 = tri[2] - tri[0];
        if (glm::length(edge1) > size || glm::length(edge2) > size)
        {
            return -1;
        }
        int child_index = -1;
        int child_index2 = -1;
        glm::vec3 node_center = node_loc + glm::vec3(size/2.0);
        std::array<int, 3> indices;
        std::array<int, 3> alt_indices;
        int i = 0;
        for (auto& coord : tri)
        {
            if (coord.x - node_loc.x < 0 || coord.x - node_loc.x > size)
            {
                return -1;
            }
            if (coord.y - node_loc.y < 0 || coord.y - node_loc.y > size)
            {
                return -1;
            }
            if (coord.z - node_loc.z < 0 || coord.z - node_loc.z > size)
            {
                return -1;
            }
            int x_index = int(coord.x > node_center.x);
            int y_index = int(coord.y > node_center.y);
            int z_index = int(coord.z > node_center.z);
            int x_index2 = int(coord.x >= node_center.x);
            int y_index2 = int(coord.y >= node_center.y);
            int z_index2 = int(coord.z >= node_center.z);
            indices[i] = x_index + y_index * 2 + z_index * 4;
            alt_indices[i] = x_index2 + y_index2 * 2 + z_index2 * 4;
            child_index = indices[i];
            child_index2 = alt_indices[i];
            child_pos = node_loc + glm::vec3(x_index, y_index, z_index) *
                    size / 2.0f;
            /*
            if (i > 0)
            {
                if (indices[i] != indices[i-1])
                {
                    child_index = -1;
                    break;
                }
            }
            */
            i += 1;
        }
        child_index = indices[0];
        child_index2 = alt_indices[0];
        for (int i = 1; i < 3; ++i)
        {
            if (indices[i] == child_index || alt_indices[i] == child_index)
            {

            }
            else
            {
                child_index = -1;
            }
            if (indices[i] == child_index2 || alt_indices[i] == child_index2)
            {

            }
            else
            {
                child_index2 = -1;
            }
        }
        if (child_index != -1)
        {
            child_pos = node_loc + glm::vec3(int(bool(child_index & 0b1)), int(bool(child_index & 0b10)), int(bool(child_index & 0b100))) *
                size / 2.0f;
            return child_index;
        }
        child_pos = node_loc + glm::vec3(int(bool(child_index2 & 0b1)), int(bool(child_index2 & 0b10)), int(bool(child_index2 & 0b100))) *
                size / 2.0f;
        return child_index2;
    }

    NodeHandle add_triangle(Triangle& tri, glm::vec3& norm, NodeHandle start_node=0,  int depth=0)
    {
        depth += 1;
        NodeHandle c_node = start_node;
        glm::vec3 child_pos;
        int child_index = child_index_of_triangle(tri,
                            _data[c_node].loc,
                            _data[c_node].size,
                            child_pos);

        // While all vertices of triangle fit in 
        // a single child of the current node
        while (child_index >= 0)
        {
            // If current node already has a child there
            // set the current node to that child
            if (_data[c_node].children[child_index] >= 0)
            {
                c_node = _data[c_node].children[child_index];
            }
            else // Node child where triangle is located, create new
            {
                int child_node = add_node(child_pos, _data[c_node].size/2.0, c_node);
                _data[c_node].children[child_index] = child_node;
                //_data[c_node].contains = true;
                c_node = child_node;
            }
            child_index = child_index_of_triangle(tri,
                            _data[c_node].loc,
                            _data[c_node].size,
                            child_pos);
        }
        bool addit = false;
        if (_data[c_node].tri_count == MAX_TRIS || largest_side(tri) < _data[c_node].size / 2)// || _data[c_node].size > 0.1)
        {
            std::vector<Triangle> tris;
            glm::vec3 new_loc;
            float new_size;
            split_triangle(tri, tris, _data[c_node].loc, _data[c_node].size);
            if (tris.size() > 1)
            {
                addit = true;
                for (auto& t : tris)
                {
                    glm::vec3 edge1 = glm::normalize(t[0] - t[1]);
                    glm::vec3 edge2 = glm::normalize(t[2] - t[1]);
                    if (std::abs(glm::dot(edge1, edge2)) >= 1.000 - 0.01)
                        continue;
                    add_triangle(t, norm, c_node, depth);
                }
            }
            else
            {
                if (_data[c_node].tri_count < MAX_TRIS)
                {
                    _last_tri += 1;
                    _data[c_node].tris[_data[c_node].tri_count] = _last_tri;

                    _tri_data[_last_tri] = tri;
                    _norm_data[_last_tri] = norm;
                    _data[c_node].tri_count += 1;
                }
                else
                {
                    //printf("Dropped TRiangle!\n");
                }
           } 
        }
        else // if (!addit)
        {
            if (_data[c_node].tri_count < MAX_TRIS)
            {
                _last_tri += 1;
                _data[c_node].tris[_data[c_node].tri_count] = _last_tri;
                if (_from_file)
                {
                    if (_tri_file.max_length < sizeof(Triangle) * _last_tri)
                    {
                        _norm_file.resize(_norm_file.max_length + 100 * sizeof(glm::vec3));
                        _tri_file.resize(_tri_file.max_length + 100 * sizeof(Triangle));
                    }
                }
                _tri_data[_last_tri] = tri;
                _norm_data[_last_tri] = norm;
                _data[c_node].tri_count += 1;
            }
        }
        return c_node;
    }

    // Need to find largest node containing a point that 
    // doesn't contain a triangle. Node doesn't need to exist
    // This is not possible if point is in triangle smallest node
    bool find_largest_non_container(glm::vec3 point, 
        glm::vec3 dir, 
        glm::vec3& node_loc, 
        float& size, 
        NodeHandle& found_handle, 
        glm::vec3& vtx, 
        glm::vec3& normal, 
        std::unordered_set<int>& checked) const
    {
        NodeHandle c_node = _root;
        glm::vec3 child_pos;
        int child_index;
        while(1)
        {
            child_index = child_index_of_point(point, _data[c_node].loc, _data[c_node].size, child_pos);
            float c_size = _data[c_node].size;
            if (child_index < 0)
            {
                printf("Point not in node!!!\n");
                print_vertex(point);
                printf("\n");
                print_vertex(_data[c_node].loc);
                printf("\n size %f\n", _data[c_node].size);
                assert(child_index >= 0);
            }
            if (_data[c_node].tri_count && checked.find(c_node) != checked.end())
            {
                checked.insert(c_node);
                for (int i = 0; i < _data[c_node].tri_count; ++i)
                {
                    glm::vec3 hit_point;
                    bool hit = RayIntersectsTriangle(point , dir, _tri_data[_data[c_node].tris[i]], hit_point);
                    //print_triangle(_tri_data[_data[c_node].tris[i]], 1);
                    if (hit)
                    {
                        if (glm::dot(hit_point - point, dir) > 0)
                        {
                            vtx = hit_point;
                            normal = _norm_data[_data[c_node].tris[i]];
                            return true;
                        }
                    }
                }
            }
            if (_data[c_node].children[child_index] >= 0)
            {
                c_node = _data[c_node].children[child_index]; 
            }
            else
            {
            /*
                printf("Point node!!!\n");
                print_vertex(point);
                printf("\n");
                print_vertex(_data[c_node].loc);
                printf("\n size %f\n", _data[c_node].size);
                */
                node_loc = child_pos;
                size = _data[c_node].size/2.0;
                return false;
            }
        }
    }

    glm::vec3 propogate_ray(glm::vec3 origin, 
        glm::vec3 dir, 
        bool& found_tri, 
        Node& found_node, 
        glm::vec3& vtx, 
        glm::vec3& normal, 
        std::unordered_set<int>& checked) const
    {
        NodeHandle found_handle;
        glm::vec3 child_pos;
        glm::vec3 node_loc;
        float size;
        found_tri = find_largest_non_container(origin, dir, node_loc, size, found_handle, vtx, normal, checked);
        if (found_tri)
        {
            //found_node = _data[found_handle];
        }
        glm::vec2 x = intersect_box(origin, dir, node_loc, size);

        glm::vec3 intersect_point = origin + dir * (x.y);
        glm::vec3 to_center_real = intersect_point - (node_loc + glm::vec3(size/2.0));
        glm::vec3 to_center = glm::abs(to_center_real);
        glm::vec3 n(glm::sign(to_center_real.x)*float(int(to_center.x >= to_center.y && to_center.x >= to_center.z)),
            glm::sign(to_center_real.y)* float(int(to_center.y > to_center.x && to_center.y >= to_center.z)),
            glm::sign(to_center_real.z)* float(int(to_center.z > to_center.x && to_center.z > to_center.y)));

        glm::vec3 n2((int(to_center.x > to_center.y&& to_center.x > to_center.z)),
            (int(to_center.y > to_center.y&& to_center.y > to_center.z)),
            (int(to_center.z > to_center.x&& to_center.z > to_center.y)));

        return origin + dir * (x.y) + n * glm::max( size * 0.01f, 0.0001f );
    }

    int size()
    {
        return _last_node + 1;
    }

    NodeHandle _last_node = 0;
private:
    std::string _octree_filepath;
    std::string _tri_filepath;
    std::string _norm_filepath;
    std::string _obj_filepath;
    boost::iostreams::mapped_file _octree_file;
    boost::iostreams::mapped_file _tri_file;
    boost::iostreams::mapped_file _norm_file;
    bool _from_file;
    size_t _length;
    Node * _data;
    Triangle* _tri_data;
    glm::vec3* _norm_data;
    Node _proto_node;
    NodeHandle _root = 0;
    
    int _last_tri = -1;

};

#endif  // TRIANGLE_OCTREE_H_
