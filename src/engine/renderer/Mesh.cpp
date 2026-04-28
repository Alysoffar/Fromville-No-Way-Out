#include "engine/renderer/Mesh.h"

#include <utility>

Mesh::~Mesh() {
	Destroy();
}

Mesh::Mesh(Mesh&& other) noexcept {
	*this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
	if (this != &other) {
		Destroy();
		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		indexCount = other.indexCount;
		other.vao = 0;
		other.vbo = 0;
		other.ebo = 0;
		other.indexCount = 0;
	}
	return *this;
}

void Mesh::Create(const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices) {
	Destroy();

	if (vertices.empty() || indices.empty()) {
		return;
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, position)));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, color)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	indexCount = indices.size();
}

void Mesh::Bind() const {
	if (vao != 0) {
		glBindVertexArray(vao);
	}
}

void Mesh::Draw() const {
	if (vao == 0 || indexCount == 0) {
		return;
	}

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Mesh::Destroy() {
	if (ebo != 0) {
		glDeleteBuffers(1, &ebo);
		ebo = 0;
	}
	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	indexCount = 0;
}

bool Mesh::IsValid() const {
	return vao != 0 && vbo != 0 && ebo != 0 && indexCount > 0;
}

std::size_t Mesh::GetIndexCount() const {
	return indexCount;
}