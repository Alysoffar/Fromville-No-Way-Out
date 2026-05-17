#pragma once

#include <glm/glm.hpp>

#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"

class Camera;

class DebugCubeRenderer {
public:
	DebugCubeRenderer();
	~DebugCubeRenderer();

	void Initialize();
	void Render(const Camera& camera, float aspectRatio);

private:
	Shader shader;
	Mesh cubeMesh;
	bool initialized = false;
};