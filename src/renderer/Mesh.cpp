#include "Mesh.h"

#include <algorithm>
#include <cstddef>
#include <unordered_set>

#include <glad/glad.h>

Mesh::Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds, std::vector<MeshTexture> texs)
	: vertices(std::move(verts)), indices(std::move(inds)), textures(std::move(texs)), VAO(0), VBO(0), EBO(0) {
	SetupMesh();
}

void Mesh::SetupMesh() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

	const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Position)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Normal)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Tangent)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Bitangent)));
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 4, GL_INT, stride, reinterpret_cast<void*>(offsetof(Vertex, BoneIDs)));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, BoneWeights)));

	glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) const {
	shader.Bind();
	std::unordered_set<std::string> boundTypes;
	unsigned int textureUnit = 0;
	for (const MeshTexture& texture : textures) {
		if (!boundTypes.insert(texture.type).second) {
			continue;
		}

		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, texture.id);
		shader.SetInt(texture.type, static_cast<int>(textureUnit));
		++textureUnit;
	}

	const bool hasNormalMap = std::any_of(textures.begin(), textures.end(), [](const MeshTexture& texture) { return texture.type == "normalMap"; });
	const bool hasEmissiveMap = std::any_of(textures.begin(), textures.end(), [](const MeshTexture& texture) { return texture.type == "emissiveMap"; });
	shader.SetBool("hasNormalMap", hasNormalMap);
	shader.SetBool("hasEmissiveMap", hasEmissiveMap);

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

void Mesh::DrawInstanced(Shader& shader, int count) const {
	shader.Bind();
	std::unordered_set<std::string> boundTypes;
	unsigned int textureUnit = 0;
	for (const MeshTexture& texture : textures) {
		if (!boundTypes.insert(texture.type).second) {
			continue;
		}

		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, texture.id);
		shader.SetInt(texture.type, static_cast<int>(textureUnit));
		++textureUnit;
	}

	const bool hasNormalMap = std::any_of(textures.begin(), textures.end(), [](const MeshTexture& texture) { return texture.type == "normalMap"; });
	const bool hasEmissiveMap = std::any_of(textures.begin(), textures.end(), [](const MeshTexture& texture) { return texture.type == "emissiveMap"; });
	shader.SetBool("hasNormalMap", hasNormalMap);
	shader.SetBool("hasEmissiveMap", hasEmissiveMap);

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr, count);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

GLuint Mesh::GetVAO() const {
	return VAO;
}