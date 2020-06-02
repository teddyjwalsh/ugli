#ifndef GRAPHICS_STRUCTURES_H_
#define GRAPHICS_STRUCTURES_H_

#include <glm/glm.hpp>

namespace graphics
{

class Vertex : public glm::vec3
{	
	using glm::vec3::vec3;
};

class Mesh
{
  public:
	std::vector<Vertex> vertices;
	std::vector<glm::vec3> normals;
};

Mesh create_cube(double edge_length=1, 
					double x=0, double y=0, double z=0)
{
	Mesh out_mesh;
	out_mesh.vertices.resize(36);
	out_mesh.normals.resize(36);
	int offset = 0;
	int n_offset = 0;

	glm::vec3 normal1(0, -1, 0);

	// First triangle

	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(0, -1, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	
	// Second triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(0, -1, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	
	// Third triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	normal1 = glm::vec3(-1, 0, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	
	// Fourth triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(-1, 0, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	
	// Fifth triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	normal1 = glm::vec3(0, 1, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Sixth triangle
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(0, 1, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Seventh triangle
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	normal1 = glm::vec3(1, 0, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Eighth triangle
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(1, 0, 0);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Ninth triangle
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(0, 0, 1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Tenth triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  0.5*edge_length + z);	
	normal1 = glm::vec3(0, 0, 1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Eleventh triangle
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	normal1 = glm::vec3(0, 0, -1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	// Eleventh triangle
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(-0.5*edge_length + x,
			  0.5*edge_length + y,
			  -0.5*edge_length + z);	
	out_mesh.vertices[offset++] = Vertex(0.5*edge_length + x,
			  -0.5*edge_length + y,
			  -0.5*edge_length + z);	
	normal1 = glm::vec3(0, 0, -1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);
	out_mesh.normals[n_offset++] = normal1/glm::length(normal1);

	return out_mesh;
}

}  // namespace graphics

#endif  // GRAPHICS_STRUCTURES_H_
