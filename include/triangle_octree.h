#ifndef TRIANGLE_OCTREE_H_
#define TRIANGLE_OCTREE_H_

#include <algorithm>
#include <array>
#include <unordered_set>
#include <filesystem>
#include "glm/glm.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include "tiny_obj_loader.h"

#include "triangle.h"
#include "split_triangle.h"

#define MAX_TRIS 20

bool RayIntersectsTriangle(const glm::vec3& rayOrigin,
                           const glm::vec3& rayVector,
                           const Triangle& inTriangle,
                           glm::vec3& outIntersectionPoint)
{
    const float EPSILON = 0.00000001;
    const glm::vec3& vertex0 = inTriangle[0];
    const glm::vec3& vertex1 = inTriangle[1];
    const glm::vec3& vertex2 = inTriangle[2];
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
        size_t length,
        const glm::vec3& offset = glm::vec3(0),
        float scale = 1.0) :
        _data(data),
        _tri_data(tri_data),
        _norm_data(norm_data),
        _length(length),
        _offset(offset),
        _scale(scale)
    {
        _proto_node.size = 10;
        _proto_node.loc = glm::vec3(0);
        _data[_root] = _proto_node;
        for (int i = 0; i < 8; ++i)
        {
            _proto_node.children[i] = -1;
        }
        _last_tri = 0;
        _from_file = false;
    }

    TriangleOctree(const std::string& octree_filepath, 
        const std::string& tri_filepath,
        const std::string& norm_filepath,
        const std::string& obj_filepath,
        const glm::vec3& offset=glm::vec3(0),
        float scale=1.0):
            _octree_filepath(octree_filepath),
            _tri_filepath(tri_filepath),
            _obj_filepath(obj_filepath),
            _norm_filepath(norm_filepath),
            _offset(offset),
            _scale(scale)
    {
        _proto_node.size = 10;
        _proto_node.loc = glm::vec3(0);
        for (int i = 0; i < 8; ++i)
        {
            _proto_node.children[i] = -1;
        }
        _last_tri = 0;
        if (set_data_from_files())
        {
            load_model();
        }
    }

    void close_files()
    {
        if (_octree_file.is_open())
        {
            _octree_file.close();
        }
        if (_tri_file.is_open())
        {
            _tri_file.close();
        }
        if (_norm_file.is_open())
        {
            _norm_file.close();
        }
    }

    bool open_files_write()
    {
        close_files();
        _octree_file.open(_octree_filepath, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::app);
        _tri_file.open(_tri_filepath, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::app);
        _norm_file.open(_norm_filepath, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::app);

        if (!_octree_file.is_open() || !_tri_file.is_open() || !_norm_file.is_open())
        {
            assert(_octree_file.is_open());
            assert(_tri_file.is_open());
            assert(_norm_file.is_open());
            return false;
        }
        _data = reinterpret_cast<Node*>(_octree_file.data());
        _tri_data = reinterpret_cast<Triangle*>(_tri_file.data());
        _norm_data = reinterpret_cast<glm::vec3*>(_norm_file.data());
        _c_data = reinterpret_cast<const Node*>(_octree_file.const_data());
        _tri_c_data = reinterpret_cast<const Triangle*>(_tri_file.const_data());
        _norm_c_data = reinterpret_cast<const glm::vec3*>(_norm_file.const_data());
        return true;
    }

    bool open_files_read()
    {
        close_files();
        _octree_file.open(_octree_filepath, std::ios_base::in | std::ios_base::binary);
        _tri_file.open(_tri_filepath, std::ios_base::in | std::ios_base::binary);
        _norm_file.open(_norm_filepath, std::ios_base::in | std::ios_base::binary);
        
        if (!_octree_file.is_open() || !_tri_file.is_open() || !_norm_file.is_open())
        {
            assert(_octree_file.is_open());
            assert(_tri_file.is_open());
            assert(_norm_file.is_open());
            return false;
        }
        _c_data = reinterpret_cast<const Node*>(_octree_file.const_data());
        _tri_c_data = reinterpret_cast<const Triangle*>(_tri_file.const_data());
        _norm_c_data = reinterpret_cast<const glm::vec3*>(_norm_file.const_data());
        return true;
    }

    bool set_data_from_files()
    {
        bool generate = false;
        if (!std::filesystem::exists(_octree_filepath))
        {
            generate = true; 
            FILE * fd = fopen(_octree_filepath.c_str(), "wb");
            fwrite(reinterpret_cast<void*>(&_proto_node), sizeof(Node), 1, fd);
            fclose(fd);
        }
        if (!std::filesystem::exists(_tri_filepath))
        {
            generate = true; 
            FILE * fd = fopen(_tri_filepath.c_str(), "wb");
            Triangle temp_tri;
            fwrite(reinterpret_cast<void*>(&temp_tri), sizeof(Triangle), 1, fd);
            fclose(fd);
        }
        if (!std::filesystem::exists(_norm_filepath))
        {
            generate = true; 
            FILE * fd = fopen(_norm_filepath.c_str(), "wb");
            glm::vec3 temp_vec;
            fwrite(reinterpret_cast<void*>(&temp_vec), sizeof(glm::vec3), 1, fd);
            fclose(fd);
        }
        auto _filesize = std::filesystem::file_size(_octree_filepath);
        auto _tri_filesize = std::filesystem::file_size(_tri_filepath);
        auto _norm_filesize = std::filesystem::file_size(_norm_filepath);
        open_files_read();
        _from_file = true;
        return generate;
    }

    void load_model()
    {
        open_files_write();
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        Triangle test_tri = {glm::vec3(110, 112, 100), glm::vec3(112,110,100), glm::vec3(110,112,102)};
        std::string err;
        std::string warn;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _obj_filepath.c_str());
        printf(warn.c_str());
        printf(err.c_str());

        if (_octree_file.is_open())
        {
            // Find bounds to create root node
            glm::vec3 max_vec;
            glm::vec3 min_vec;
            for (size_t s = 0; s < shapes.size(); s++) 
            {
              // Loop over faces(polygon)
              size_t index_offset = 0;
              
              for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
              {
                int fv = shapes[s].mesh.num_face_vertices[f];
                glm::vec3 norm;
                glm::vec3 center(0);
                for (size_t v = 0; v < fv; v++) 
                {
                  tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                  tinyobj::real_t vx = attrib.vertices[3*idx.vertex_index+0];
                  tinyobj::real_t vy = attrib.vertices[3*idx.vertex_index+1];
                  tinyobj::real_t vz = attrib.vertices[3*idx.vertex_index+2];
                  if (f == 0 && s == 0 && v == 0)
                  {
                    min_vec.x = vx; min_vec.y = vy; min_vec.z = vz;
                    max_vec = min_vec;
                  }
                  if (vx > max_vec.x)
                  {
                    max_vec.x = vx;
                  }
                  if (vy > max_vec.y)
                  {
                    max_vec.y = vy;
                  }
                  if (vz > max_vec.z)
                  {
                    max_vec.z = vz;
                  }
                  if (vx < min_vec.x)
                  {
                    min_vec.x = vx;
                  }
                  if (vy < min_vec.y)
                  {
                    min_vec.y = vy;
                  }
                  if (vz < min_vec.z)
                  {
                    min_vec.z = vz;
                  }
                }
                index_offset += fv;
              }
            }
            glm::vec3 diff = max_vec - min_vec;
            _proto_node.size = std::max({diff.x, diff.y, diff.z});
            _proto_node.loc = min_vec;
            printf("Root Node:\n");
            print_vertex(_proto_node.loc);
            printf("\n\t%f\n", _proto_node.size);
            _data[_root] = _proto_node;


            // Add triangles to octree
            //    Loop over shapes
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
                float scale = _scale;///15.0;
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
                  test_tri[v][0] = scale * vx + _offset.x;
                  test_tri[v][1] = scale * vy + _offset.y;
                  test_tri[v][2] = scale * vz + _offset.z;
                  //test_tri[v][2] = scale*vz + 9.0;
                }
                norm = glm::normalize(glm::cross(test_tri[1] - test_tri[0], test_tri[0] - test_tri[2]));
                float root_size = _data[0].size;
                add_triangle(test_tri, norm);
                if (root_size != _data[0].size)
                {
                    printf("ROOT CORRUPETED!!!!\n");
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
              }
            }
            printf("model tree created\n");
        }
        open_files_read();
    }

    float largest_side(const Triangle& tri) const
    {
        return std::max({ glm::length(tri[2] - tri[0]),
            glm::length(tri[1] - tri[0]),
            glm::length(tri[2] - tri[1]) });
    }

    glm::vec2 intersect_box(const glm::vec3& origin, const glm::vec3& dir, 
                            const glm::vec3& box_origin, float box_size) const
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

    NodeHandle add_node(const glm::vec3& loc, float size, NodeHandle parent)
    {
        _last_node += 1;
        _proto_node.size = size;
        _proto_node.loc = loc;
        //_proto_node.parent = parent;
        if (_from_file)
        {
            if (_filesize < sizeof(Node) * (_last_node-20))
            {
                float root_size = _data[_root].size;
                _octree_file.resize((_last_node + 3000000)*sizeof(Node));
                _data = reinterpret_cast<Node*>(_octree_file.data());
                if (root_size != _data[_root].size)
                {
                    printf("Root node corrupted!\n");
                }
                _filesize = (_last_node + 3000000)*sizeof(Node);
            }
        }
        //_data[_last_node] = _proto_node;
        reinterpret_cast<Node*>(_octree_file.data())[_last_node] = _proto_node;
        return _last_node;
    }

    int child_index_of_point(const glm::vec3& coord, 
        const glm::vec3& node_loc, 
        float size, 
        glm::vec3& child_pos) const
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

    int child_index_of_triangle(const Triangle& tri, 
        const glm::vec3& node_loc, 
        float size, 
        glm::vec3& child_pos) const
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

    NodeHandle add_triangle(const Triangle& tri, 
        const glm::vec3& norm, 
        NodeHandle start_node=0,  
        int depth=0)
    {
        depth += 1;
        NodeHandle c_node = start_node;
        glm::vec3 child_pos;
        float root_size = _data[0].size;
        int child_index = child_index_of_triangle(tri,
                            _data[c_node].loc,
                            _data[c_node].size,
                            child_pos);

        // While all vertices of triangle fit in 
        // a single child of the current node
        if (root_size != _data[0].size)
        {
            printf("CORRUPT 0\n");
        }
        while (child_index >= 0)
        {
            // If current node already has a child there
            // set the current node to that child
            if (_data[c_node].children[child_index] >= 0)
            {
                c_node = _data[c_node].children[child_index];
                if (c_node == 0)
                {
                    printf("ZERO CHILD~!!!\n");
                }
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
        if (root_size != _data[0].size)
        {
            printf("CORRUPT 1\n");
        }
        if (_data[c_node].tri_count == MAX_TRIS || largest_side(tri) < _data[c_node].size / 2)// || _data[c_node].size > 0.1)
        {
            std::vector<Triangle> tris;
            glm::vec3 new_loc;
            float new_size;
            split_triangle(tri, tris, _data[c_node].loc, _data[c_node].size);
                if (root_size != _data[0].size)
                {
                    printf("CORRUPT 1.01\n");
                }
            if (tris.size() > 1)
            {
                for (auto& t : tris)
                {
                    glm::vec3 edge1 = glm::normalize(t[0] - t[1]);
                    glm::vec3 edge2 = glm::normalize(t[2] - t[1]);
                    if (std::abs(glm::dot(edge1, edge2)) >= 1.000 - 0.01)
                        continue;
                    add_triangle(t, norm, c_node, depth);
                }
                if (root_size != _data[0].size)
                {
                    printf("CORRUPT 1.1\n");
                }
            }
            else
            {
                if (_data[c_node].tri_count < MAX_TRIS)
                {
                    _last_tri += 1;
                    _data[c_node].tris[_data[c_node].tri_count] = _last_tri;
                if (root_size != _data[0].size)
                {
                    printf("CORRUPT 1.x\n");
                }
                    if (_from_file)
                    {
                        if (_tri_filesize < sizeof(Triangle) * (_last_tri - 5))
                        {
                            printf("Resize\n");
                            _tri_file.resize((_last_tri + 1000000)*sizeof(Triangle));
                            _tri_data = reinterpret_cast<Triangle*>(_tri_file.data());
                            _tri_filesize = (_last_tri + 1000000)*sizeof(Triangle);
                        }
                        if (_norm_filesize < sizeof(glm::vec3) * (_last_tri - 5))
                        {
                            printf("Resize norm\n");
                            _norm_file.resize((_last_tri + 1000000)*sizeof(glm::vec3));
                            _norm_data = reinterpret_cast<glm::vec3*>(_norm_file.data());
                            _norm_filesize = (_last_tri + 1000000)*sizeof(glm::vec3);
                        }
                    }

                    while (!_norm_file.is_open() || !_tri_file.is_open())
                    {}
                    reinterpret_cast<Triangle*>(_tri_file.data())[_last_tri] = tri;
                    reinterpret_cast<glm::vec3*>(_norm_file.data())[_last_tri] = norm;
                    _data[c_node].tri_count += 1;
                }
                else
                {
                    //printf("Dropped TRiangle!\n");
                }
                if (root_size != _data[0].size)
                {
                    printf("CORRUPT 1.2\n");
                }
           } 
            if (root_size != _data[0].size)
            {
                printf("CORRUPT 2.1\n");
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
                    if (_tri_filesize < sizeof(Triangle) * (_last_tri - 5))
                    {
                        printf("Resize\n");
                        _tri_file.resize((_last_tri + 10000000)*sizeof(Triangle));
                        _tri_data = reinterpret_cast<Triangle*>(_tri_file.data());
                        _tri_filesize = (_last_tri + 10000000)*sizeof(Triangle);
                    }
                    if (_norm_filesize < sizeof(glm::vec3) * (_last_tri - 5))
                    {
                        printf("Resize norm\n");
                        _norm_file.resize((_last_tri + 10000000)*sizeof(glm::vec3));
                        _norm_data = reinterpret_cast<glm::vec3*>(_norm_file.data());
                        _norm_filesize = (_last_tri + 10000000)*sizeof(glm::vec3);
                    }
                }

                while (!_norm_file.is_open() || !_tri_file.is_open())
                {}
                reinterpret_cast<Triangle*>(_tri_file.data())[_last_tri] = tri;
                reinterpret_cast<glm::vec3*>(_norm_file.data())[_last_tri] = norm;
                _data[c_node].tri_count += 1;
            }
            if (root_size != _data[0].size)
            {
                printf("CORRUPT 2.2\n");
            }
        }
        if (root_size != _data[0].size)
        {
            printf("CORRUPT 2\n");
        }
        return c_node;
    }

    // Need to find largest node containing a point that 
    // doesn't contain a triangle. Node doesn't need to exist
    // This is not possible if point is in triangle smallest node
    bool find_largest_non_container(const glm::vec3& point, 
        const glm::vec3& dir, 
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
            child_index = child_index_of_point(point, _c_data[c_node].loc, _c_data[c_node].size, child_pos);
            float c_size = _c_data[c_node].size;
            if (child_index < 0)
            {
                node_loc = _c_data[c_node].loc;
                size = _c_data[c_node].size;
                found_handle = Node::invalid_handle;
                return false;
            }
            if (_c_data[c_node].tri_count && checked.find(c_node) == checked.end())
            {
                checked.insert(c_node);
                for (int i = 0; i < _c_data[c_node].tri_count; ++i)
                {
                    glm::vec3 hit_point;
                    bool hit = RayIntersectsTriangle(point, dir, _tri_c_data[_c_data[c_node].tris[i]], hit_point);
                    if (hit)
                    {
                        if (glm::dot(hit_point - point, dir) > 0)
                        {
                            vtx = hit_point;
                            normal = _norm_c_data[_c_data[c_node].tris[i]];
                            found_handle = c_node; 
                            return true;
                        }
                    }
                }
            }
            if (_c_data[c_node].children[child_index] >= 0)
            {
                c_node = _c_data[c_node].children[child_index]; 
            }
            else
            {
                node_loc = child_pos;
                size = _c_data[c_node].size/2.0;
                found_handle = c_node;
                return false;
            }
        }
    }

    glm::vec3 propogate_ray(const glm::vec3& origin_in, 
        const glm::vec3& dir, 
        bool& found_tri, 
        glm::vec3& vtx, 
        glm::vec3& normal, 
        std::unordered_set<int>& checked,
        bool& missed) const
    {
        glm::vec3 origin = origin_in - _offset;
        missed = false;
        NodeHandle found_handle;
        glm::vec3 child_pos;
        glm::vec3 node_loc;
        float size;
        found_tri = find_largest_non_container(origin, dir, node_loc, size, found_handle, vtx, normal, checked);
        vtx += _offset;
        glm::vec2 x = intersect_box(origin, dir, node_loc, size);
        if (found_handle == Node::invalid_handle)
        {
            glm::vec3 intersect_point = origin + dir * (x.x);
            glm::vec3 to_center_real = intersect_point - (node_loc + glm::vec3(size / 2.0));
            glm::vec3 to_center = glm::abs(to_center_real);
            glm::vec3 n(glm::sign(to_center_real.x) * float(int(to_center.x >= to_center.y && to_center.x >= to_center.z)),
                glm::sign(to_center_real.y) * float(int(to_center.y > to_center.x&& to_center.y >= to_center.z)),
                glm::sign(to_center_real.z)* float(int(to_center.z > to_center.x&& to_center.z > to_center.y)));
            if (x.x > 0 && x.x < x.y)
            {
                return origin + dir * (x.x) + (-n) * glm::max(size * 0.01f, 0.0001f) + _offset;
            }
            else
            {
                missed = true;
                return glm::vec3(0);
            }
        }
        else if (found_handle == _root)
        {
            missed = true;
            return glm::vec3(0);
            
        }
        else
        {
            glm::vec3 intersect_point = origin + dir * (x.y);
            glm::vec3 to_center_real = intersect_point - (node_loc + glm::vec3(size / 2.0));
            glm::vec3 to_center = glm::abs(to_center_real);
            glm::vec3 n(glm::sign(to_center_real.x) * float(int(to_center.x >= to_center.y && to_center.x >= to_center.z)),
                glm::sign(to_center_real.y) * float(int(to_center.y > to_center.x&& to_center.y >= to_center.z)),
                glm::sign(to_center_real.z)* float(int(to_center.z > to_center.x&& to_center.z > to_center.y)));

            return origin + dir * (x.y) + n * glm::max(size * 0.01f, 0.0001f) + _offset;
        }
        
    }

    int size() const
    {
        return _last_node + 1;
    }

    NodeHandle _last_node = 0;
    glm::vec3 _offset;
    float _scale;
private:
    std::string _octree_filepath;
    std::string _tri_filepath;
    std::string _norm_filepath;
    std::string _obj_filepath;
    boost::iostreams::mapped_file _octree_file;
    boost::iostreams::mapped_file _tri_file;
    boost::iostreams::mapped_file _norm_file;
    int _filesize;
    int _tri_filesize;
    int _norm_filesize;
    bool _from_file;
    size_t _length;

    Node * _data;
    Triangle* _tri_data;
    glm::vec3* _norm_data;

    const Node* _c_data;
    const Triangle* _tri_c_data;
    const glm::vec3* _norm_c_data;

    Node _proto_node;
    NodeHandle _root = 0;
    int _last_tri = -1;

};

#endif  // TRIANGLE_OCTREE_H_
