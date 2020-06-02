#ifndef GRAPHICS_OBJECT_H_
#define GRAPHICS_OBJECT_H_

#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/string_cast.hpp"

#include "graphics/gl_includes.h"
#include "graphics/entity.h"
#include "graphics/buffer.h"
#include "graphics/structures.h"
#include "graphics/shader.h"
#include "graphics/definitions.h"

namespace graphics
{

class Object : public Entity
{
  public:
	Object():
		_is_instanced(false),
		_vertices(std::make_shared<Buffer<Vertex>>()),
		_normals(std::make_shared<Buffer<glm::vec3>>()),
		_instances(std::make_shared<Buffer<glm::vec3>>(GL_ARRAY_BUFFER, true))
	{
		glGenVertexArrays(1, &_vertex_array);	
	}

	void load_mesh(Mesh& in_mesh)
	{
		load_vertices(in_mesh.vertices);
		load_normals(in_mesh.normals);
	}

	void load_vertices(std::vector<Vertex> vertices)
	{
		_vertices->load_data(vertices);
	}

	void load_instances(std::vector <glm::vec3> instances)
	{
		_instances->load_data(instances);

		enable_instancing();
	}

	void load_normals(std::vector<glm::vec3> normals)
	{
		_normals->load_data(normals);
	}

	void enable_instancing(bool enable=true)
	{
		bind();
		_instances->bind();
		glBindAttribLocation(_program->get_program_name(),
			_INSTANCE_BINDING,
			DEFAULT_INSTANCE_VARIABLE.c_str());
		glEnableVertexAttribArray(_INSTANCE_BINDING);
		
		glVertexAttribPointer(_INSTANCE_BINDING, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		glVertexAttribDivisor(_INSTANCE_BINDING, 1);
		_program->link();

		_is_instanced = enable;
	}
	
	bool is_instanced()
	{
		return _is_instanced;
	}

	void bind() const
	{
		glBindVertexArray(_vertex_array);
	}

	int vertex_count()
	{
		return _vertices->count();
	}

	void set_shader(std::shared_ptr<Program> program)
	{
		bind();
		_program = program;

		_vertices->bind();
		glBindAttribLocation(_program->get_program_name(), 
			_POSITION_BINDING, 
			DEFAULT_VERTEX_POSITION_VARIABLE.c_str());
		glEnableVertexAttribArray(_POSITION_BINDING);
		glVertexAttribPointer(_POSITION_BINDING, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
		_normals->bind();
		glBindAttribLocation(_program->get_program_name(),
			_NORMAL_BINDING,
			DEFAULT_VERTEX_NORMAL_VARIABLE.c_str());
		glEnableVertexAttribArray(_NORMAL_BINDING);
		glVertexAttribPointer(_NORMAL_BINDING, 3, GL_FLOAT, GL_FALSE, 0, 0);
		_program->link();
	}

	void use_program()
	{
		_program->use();
	}

	void draw()
	{
		bind();
		GLuint model_mat_loc = glGetUniformLocation(_program->get_program_name(), "model_mat");
		glm::mat4 model_mat = get_transform();
		glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, glm::value_ptr(model_mat));
		if (_is_instanced)
		{
			glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_count(), get_instance_count());
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, vertex_count());
		}
	}

	std::shared_ptr<Program> get_program() const
	{
		return _program;
	}

	int get_instance_count() const
	{
		return _instances->count();
	}

	std::shared_ptr<Buffer<Vertex>> _vertices;
	std::shared_ptr <Buffer<glm::vec3>> _normals;
	std::shared_ptr <Buffer<glm::vec3>> _instances;
  private:
	bool _is_instanced;
	GLuint _vertex_array;
	std::shared_ptr<Program> _program;

	int _POSITION_BINDING = 0;
	int _NORMAL_BINDING = 1;
	int _INSTANCE_BINDING = 2;
};

}  // namespace graphics

#endif  // GRAPHICS_OBJECT_H_

