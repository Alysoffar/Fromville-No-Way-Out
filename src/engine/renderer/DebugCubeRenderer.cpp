#include "engine/renderer/DebugCubeRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"

DebugCubeRenderer::DebugCubeRenderer()
	: shader("DebugCube") {
}

DebugCubeRenderer::~DebugCubeRenderer() {
}

void DebugCubeRenderer::Initialize() {
	if (initialized) {
		return;
	}

	shader.Load("assets/shaders/basic_cube.vert", "assets/shaders/basic_cube.frag");

	const std::vector<MeshVertex> vertices = {
		MeshVertex{glm::vec3(-0.5f, -0.5f, -0.5f)},
		MeshVertex{glm::vec3( 0.5f, -0.5f, -0.5f)},
		MeshVertex{glm::vec3( 0.5f,  0.5f, -0.5f)},
		MeshVertex{glm::vec3(-0.5f,  0.5f, -0.5f)},
		MeshVertex{glm::vec3(-0.5f, -0.5f,  0.5f)},
		MeshVertex{glm::vec3( 0.5f, -0.5f,  0.5f)},
		MeshVertex{glm::vec3( 0.5f,  0.5f,  0.5f)},
		MeshVertex{glm::vec3(-0.5f,  0.5f,  0.5f)}
	};

	const std::vector<unsigned int> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		4, 7, 3, 3, 0, 4,
		1, 5, 6, 6, 2, 1,
		4, 5, 1, 1, 0, 4,
		3, 2, 6, 6, 7, 3
	};

	cubeMesh.Create(vertices, indices);

	initialized = true;
}

void DebugCubeRenderer::Render(const Camera& camera, float aspectRatio) {
	if (!initialized) {
		return;
	}

	const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
	const glm::mat4 view = camera.GetViewMatrix();
	const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -5.0f));

	shader.Bind();
	shader.SetMat4("projection", projection);
	shader.SetMat4("view", view);
	shader.SetMat4("model", model);
	shader.SetVec3("color", glm::vec3(1.0f));

	cubeMesh.Draw();

	shader.Unbind();
}