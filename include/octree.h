#pragma once
#ifndef GRAPHICS_OCTREE_H_
#define GRAPHICS_OCTREE_H_


#include <deque>
#include <vector>
#include <glm/glm.hpp>

class Octree
{
public:
    struct Node
    {
        float size; // side sizee
        float pad2[2];
        glm::vec4 pos; // "floor" corner of bounds
        int children[8];
        glm::vec4 value;
        int parent;
        int pad[3];
    };

    Octree()
    {
        _oct_proto.parent = -1;
        for (int i = 0; i < 8; ++i)
        {
            _oct_proto.children[i] = -1;
        }
        _oct_buf.push_back(_oct_proto);
        _oct_buf.back().pos = glm::vec4(0, 0, 0, 0);
        _oct_buf.back().size = 1000;
    }

    int lookup(int depth, glm::vec3 coord, bool allocate_node)
    {
        int current_index = 0;
        for (int i = 0; i < depth; ++i)
        {
            glm::vec3 node_center = glm::vec3(_oct_buf[current_index].pos) + glm::vec3(_oct_buf[current_index].size / 2.0);
            int x_index = int(coord.x > node_center.x);
            int y_index = int(coord.y > node_center.y);
            int z_index = int(coord.z > node_center.z);
            int child_index = x_index + y_index * 2 + z_index * 4;

            if (_oct_buf[current_index].children[child_index] < 0)
            {
                if (allocate_node)
                {
                    int new_index = allocate(current_index);
                    if (new_index == -1)
                    {
                        return current_index;
                    }
                    _oct_buf[current_index].children[child_index] = new_index;
                    _oct_buf[new_index].size =
                        _oct_buf[current_index].size / 2.0;
                    _oct_buf[new_index].pos =
                        glm::vec4(glm::vec3(_oct_buf[current_index].pos) + glm::vec3(x_index, y_index, z_index) *
                        _oct_buf[current_index].size / 2.0f, 0);
                    _oct_buf[new_index].value = glm::vec4(-1000);
                }
                else
                {
                    return current_index;
                }
            }
            current_index = _oct_buf[current_index].children[child_index];
        }

        return current_index;
    }

    void set(int depth, glm::vec3 coord, glm::vec4 val)
    {
        int current_index = lookup(depth, coord, true);
        _oct_buf[current_index].value = val;
    }

    int allocate(int parent)
    {
        if (_free_nodes.size())
        {
            int fn = _free_nodes.back();
            _free_nodes.pop_back();
            _oct_buf[fn].parent = parent;
            return fn;
        }
        _oct_buf.push_back(_oct_proto);
        _oct_values.push_back(glm::vec4(-10));
        _oct_buf.back().parent = parent;
        return _oct_buf.size() - 1;
    }

    void cleanup(glm::vec3 pos, float size_thresh)
    {
        for (int i = 0; i < _oct_buf.size(); ++i)
        {
            auto& node = _oct_buf[i];
            if (node.size < size_thresh && glm::length(pos - glm::vec3(node.pos)) > 200)
            {
                for (int j = 0; j < 8; ++j)
                {
                    if (_oct_buf[node.parent].children[j] == i)
                    {
                        _oct_buf[node.parent].children[j] = -1;
                        break;
                    }
                }
                node = _oct_proto;
                _free_nodes.push_front(i);
            }
        }
    }

    void reset()
    {
        _oct_buf.clear();
        _oct_buf.push_back(_oct_proto);
        _oct_values.push_back(glm::vec4(-10));
        _oct_buf.back().pos = glm::vec4(0, 0, 0, 0);
        _oct_buf.back().size = 1000;
    }

    Node * _oct_array;
    std::vector<Node> _oct_buf;
    std::vector<glm::vec4> _oct_values;
private:
    std::deque<int> _free_nodes;
    Node _oct_proto;
};

class MappedOctree
{
public:
    struct Node
    {
        float size; // side size
        float pad[3];
        glm::vec4 pos; // "floor" corner of bounds
        int children[8];
        glm::vec4 value;
        int parent;
        int pad2[3];
    };

    MappedOctree():
        _oct_buf(std::make_shared<graphics::Buffer<Node>>(GL_SHADER_STORAGE_BUFFER)),
        _mapped_depth(0)
    {
        _oct_proto.parent = -1;
        for (int i = 0; i < 8; ++i)
        {
            _oct_proto.children[i] = -1;
        }
        std::vector<Node> oct_vector;
        oct_vector.push_back(_oct_proto);
        oct_vector.back().pos = glm::vec4(0, 0, 0, 0);
        oct_vector.back().size = 1000;
        _oct_buf->load_data(oct_vector, 10000000);
    }

    int lookup(int depth, glm::vec3 coord, bool allocate_node)
    {
        int current_index = 0;
        for (int i = 0; i < depth; ++i)
        {
            glm::vec3 node_center = glm::vec3(_oct_array[current_index].pos) + glm::vec3(_oct_array[current_index].size / 2.0);
            int x_index = int(coord.x > node_center.x);
            int y_index = int(coord.y > node_center.y);
            int z_index = int(coord.z > node_center.z);
            int child_index = x_index + y_index * 2 + z_index * 4;

            if (_oct_array[current_index].children[child_index] < 0)
            {
                if (allocate_node)
                {
                    int new_index = allocate(current_index);
                    if (new_index == -1)
                    {
                        return current_index;
                    }
                    _oct_array[current_index].children[child_index] = new_index;
                    _oct_array[new_index].size =
                        _oct_array[current_index].size / 2.0;
                    _oct_array[new_index].pos =
                        glm::vec4(glm::vec3(_oct_array[current_index].pos) + glm::vec3(x_index, y_index, z_index) *
                            _oct_array[current_index].size / 2.0f, 0);
                    _oct_array[new_index].value = _oct_array[current_index].value;// glm::vec4(-1000);
                }
                else
                {
                    return current_index;
                }
            }
            current_index = _oct_array[current_index].children[child_index];
        }
        return current_index;
    }

    void set(int depth, glm::vec3 coord, glm::vec4 val)
    {
        //map();
        int current_index = lookup(depth, coord, true);
        _oct_array[current_index].value = val;
        //unmap();
    }

    int allocate(int parent)
    {
        //map();
        if (_free_nodes.size())
        {
            int fn = _free_nodes.back();
            _free_nodes.pop_back();
            _oct_array[fn].parent = parent;
            //unmap();
            return fn;
        }
        if (_oct_buf->max_count() - 1 > _oct_buf->count())
        {
            Node temp_node = _oct_proto;
            temp_node.parent = parent;
            _oct_buf->add_data(temp_node, 10000);
        }
        else
        {
            return -1;
            unmap(true);
            Node temp_node = _oct_proto;
            temp_node.parent = parent;
            _oct_buf->add_data(temp_node, 10000);
            map(true);
        }
        //unmap();
        return _oct_buf->count() - 1;
    }

    void cleanup(glm::vec3 pos, float size_thresh)
    {
        for (int i = 2; i < _oct_buf->count(); ++i)
        {
            //auto& node = _oct_array[i];
            if (_oct_array[i].size < glm::length(pos - glm::vec3(_oct_array[i].pos)) / 1000.0 && _oct_array[i].parent > 0)//_oct_array[i].parent > 0 && _oct_array[i].size < 1 && 0);// glm::length(pos - glm::vec3(_oct_array[i].pos)) / 1000.0 && _oct_array[i].size < 20);//&& glm::length(pos - glm::vec3(_oct_array[i].pos)) > 100)
            {
                delete_node(i);
            }
        }
    }

    void delete_node(int node, int depth = 0)
    {
        if (_oct_array[node].parent >= 0 && _oct_array[node].parent < _oct_buf->count())
        {
            for (int j = 0; j < 8; ++j)
            {
                if (_oct_array[_oct_array[node].parent].children[j] == node)
                {
                    _oct_array[_oct_array[node].parent].children[j] = -1;
                    break;
                }
            }
        }
        for (int j = 0; j < 8; ++j)
        {
            if (_oct_array[node].children[j] > 0 && 
                _oct_array[node].children[j] < _oct_buf->count() &&
                depth < 15)
            {
                _oct_array[_oct_array[node].children[j]].parent = -1;
                delete_node(_oct_array[node].children[j], depth + 1);
            }
        }
        _oct_array[node] = _oct_proto;
        _free_nodes.push_front(node);
    }

    void reset()
    {
        std::vector<Node> oct_vector;
        oct_vector.push_back(_oct_proto);
        oct_vector.back().pos = glm::vec4(0, 0, 0, 0);
        oct_vector.back().size = 1000;
        _oct_buf->load_data(oct_vector, 10000);
    }

    void map(bool force=false)
    {
        if (force)
        {
            _oct_array = _oct_buf->map_memory();
            return;
        }
        if (_mapped_depth <= 0)
        {
            _oct_array = _oct_buf->map_memory();
        }
        _mapped_depth += 1;
    }

    void unmap(bool force=false)
    {
        if (force)
        {
            _oct_buf->unmap_memory();
            _oct_array = nullptr;
        }
        if (_mapped_depth > 0)
        {
            if (_mapped_depth == 1)
            {
                _oct_buf->unmap_memory();
                _oct_array = nullptr;
            }
            _mapped_depth -= 1;
        }
    }

    std::shared_ptr<graphics::Buffer<Node>> get_buffer()
    {
        return _oct_buf;
    }

    Node* _oct_array;
    std::shared_ptr<graphics::Buffer<Node>> _oct_buf;
    std::vector<glm::vec4> _oct_values;
private:
    std::deque<int> _free_nodes;
    Node _oct_proto;
    int _mapped_depth;
};

#endif GRAPHICS_OCTREE_H_