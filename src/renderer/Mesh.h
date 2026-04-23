#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Shader.h"

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	glm::ivec4 BoneIDs = glm::ivec4(0);
	glm::vec4 BoneWeights = glm::vec4(0.0f);
};

struct MeshTexture {
	GLuint id;
	std::string type;
	std::string path;
};

class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<MeshTexture> textures;

	Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds, std::vector<MeshTexture> texs);
	void Draw(Shader& shader) const;
	void DrawInstanced(Shader& shader, int count) const;
	GLuint GetVAO() const;

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	void SetupMesh();
};