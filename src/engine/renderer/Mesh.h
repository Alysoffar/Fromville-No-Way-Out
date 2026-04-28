#pragma once

#include <cstddef>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct MeshVertex {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
	glm::vec3 color = glm::vec3(0.9f);
};

class Mesh {
public:
	Mesh() = default;
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Create(const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices);
	void Bind() const;
	void Draw() const;
	void Destroy();

	bool IsValid() const;
	std::size_t GetIndexCount() const;

private:
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	std::size_t indexCount = 0;
};