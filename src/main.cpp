#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Narrative system test suite (headless — runs before the window opens)
int RunNarrativeTests();

#include "ai/CreatureAI.h"
#include "ai/NavMesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "characters/Boyd.h"
#include "characters/Jade.h"
#include "characters/Sara.h"
#include "characters/Tabitha.h"
#include "characters/Victor.h"

#include "renderer/Model.h"
#include "renderer/Shader.h"

#include "core/Timer.h"
#include "core/Window.h"

namespace {
const char* CreatureStateName(CreatureState s) {
	switch (s) {
	case CreatureState::DORMANT: return "DORMANT";
	case CreatureState::PATROL: return "PATROL";
	case CreatureState::SEARCH: return "SEARCH";
	case CreatureState::HUNT: return "HUNT";
	case CreatureState::WHISPER: return "WHISPER";
	case CreatureState::RETREAT: return "RETREAT";
	default: return "UNKNOWN";
	}
}

glm::vec3 CreatureStateColor(CreatureState s) {
	switch (s) {
	case CreatureState::DORMANT: return glm::vec3(0.35f, 0.35f, 0.35f);
	case CreatureState::PATROL: return glm::vec3(0.25f, 0.75f, 0.35f);
	case CreatureState::SEARCH: return glm::vec3(0.95f, 0.80f, 0.20f);
	case CreatureState::HUNT: return glm::vec3(0.95f, 0.20f, 0.20f);
	case CreatureState::WHISPER: return glm::vec3(0.55f, 0.25f, 0.85f);
	case CreatureState::RETREAT: return glm::vec3(0.20f, 0.85f, 0.95f);
	default: return glm::vec3(1.0f);
	}
}

std::unique_ptr<Model> TryLoadFirstModel(const std::vector<std::string>& candidates, std::string& pickedPath) {
	for (const std::string& path : candidates) {
		if (!std::filesystem::exists(path)) {
			continue;
		}
		try {
			pickedPath = path;
			return std::make_unique<Model>(path, true);
		} catch (...) {
			pickedPath.clear();
		}
	}
	return nullptr;
}

GLuint CompileShader(GLenum type, const char* source) {
	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[1024] = {};
		glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
		std::fprintf(stderr, "Shader compile error: %s\n", infoLog);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
	const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (!vertexShader || !fragmentShader) {
		if (vertexShader) glDeleteShader(vertexShader);
		if (fragmentShader) glDeleteShader(fragmentShader);
		return 0;
	}

	const GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[1024] = {};
		glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
		std::fprintf(stderr, "Program link error: %s\n", infoLog);
		glDeleteProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}
}

int main() {
	// ── Headless tests (audio + narrative) ── run before GL context
	const int testResult = RunNarrativeTests();
	if (testResult != 0) {
		std::fprintf(stderr, "[main] Narrative tests FAILED — aborting launch\n");
		return testResult;
	}
	std::printf("[main] All narrative tests passed.\n");

	Window window(1280, 720, "Fromville: No Way Out");
	if (!window.Init()) {
		std::fprintf(stderr, "Failed to initialize Fromville window\n");
		return 1;
	}

	const char* vertexSource = R"(
		#version 450 core
		layout (location = 0) in vec3 aPos;
		uniform mat4 uModel;
		uniform mat4 uView;
		uniform mat4 uProjection;
		void main() {
			gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
		}
	)";

	const char* fragmentSource = R"(
		#version 450 core
		out vec4 FragColor;
		uniform vec3 uColor;
		void main() {
			FragColor = vec4(uColor, 1.0);
		}
	)";

	const GLuint characterProgram = CreateProgram(vertexSource, fragmentSource);
	if (!characterProgram) {
		std::fprintf(stderr, "Failed to create character debug shader program\n");
		return 1;
	}

	// 3D cube proxy for each character.
	const float cubeVertices[] = {
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
	};

	const unsigned int cubeIndices[] = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		0, 4, 7, 7, 3, 0,
		1, 5, 6, 6, 2, 1,
		3, 2, 6, 6, 7, 3,
		0, 1, 5, 5, 4, 0,
	};

	GLuint cubeVAO = 0;
	GLuint cubeVBO = 0;
	GLuint cubeEBO = 0;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glBindVertexArray(0);

	std::vector<std::unique_ptr<Character>> characters;
	characters.emplace_back(std::make_unique<Boyd>());
	characters.emplace_back(std::make_unique<Jade>());
	characters.emplace_back(std::make_unique<Tabitha>());
	characters.emplace_back(std::make_unique<Victor>());
	characters.emplace_back(std::make_unique<Sara>());
	std::vector<std::unique_ptr<Model>> characterModels;
	characterModels.resize(characters.size());

	const std::vector<glm::vec3> characterColors = {
		glm::vec3(0.85f, 0.70f, 0.25f), // Boyd
		glm::vec3(0.90f, 0.45f, 0.20f), // Jade
		glm::vec3(0.35f, 0.70f, 0.95f), // Tabitha
		glm::vec3(0.95f, 0.90f, 0.70f), // Victor
		glm::vec3(0.70f, 0.35f, 0.90f), // Sara
	};

	for (std::size_t i = 0; i < characters.size(); ++i) {
		characters[i]->position = glm::vec3(-6.0f + static_cast<float>(i) * 3.0f, 0.75f, 0.0f);
		characters[i]->SetNoiseLevelThisFrame(3.0f);
	}

	std::vector<std::vector<std::string>> characterModelCandidates = {
		{"assets/models/boyd.glb", "assets/models/boyd.gltf", "assets/models/boyd.fbx", "assets/models/boyd.obj"},
		{"assets/models/jade.glb", "assets/models/jade.gltf", "assets/models/jade.fbx", "assets/models/jade.obj"},
		{"assets/models/tabitha.glb", "assets/models/tabitha.gltf", "assets/models/tabitha.fbx", "assets/models/tabitha.obj"},
		{"assets/models/victor.glb", "assets/models/victor.gltf", "assets/models/victor.fbx", "assets/models/victor.obj"},
		{"assets/models/sara.glb", "assets/models/sara.gltf", "assets/models/sara.fbx", "assets/models/sara.obj"},
	};

	for (std::size_t i = 0; i < characters.size(); ++i) {
		std::string picked;
		characterModels[i] = TryLoadFirstModel(characterModelCandidates[i], picked);
		characters[i]->model = characterModels[i].get();
		if (!characterModels[i]) {
			std::printf("Model fallback: using cube for %s (no asset found)\n", characters[i]->GetCharacterName().c_str());
		} else {
			std::printf("Loaded model for %s: %s\n", characters[i]->GetCharacterName().c_str(), picked.c_str());
		}
	}

	auto creatureHumanModel = std::unique_ptr<Model>();
	auto creatureMonsterModel = std::unique_ptr<Model>();
	{
		std::string picked;
		creatureHumanModel = TryLoadFirstModel(
			{"assets/models/creature_human.glb", "assets/models/creature_human.gltf", "assets/models/creature_human.fbx", "assets/models/creature_human.obj"},
			picked);
		if (creatureHumanModel) {
			std::printf("Loaded creature human model: %s\n", picked.c_str());
		}
	}
	{
		std::string picked;
		creatureMonsterModel = TryLoadFirstModel(
			{"assets/models/creature_monster.glb", "assets/models/creature_monster.gltf", "assets/models/creature_monster.fbx", "assets/models/creature_monster.obj"},
			picked);
		if (creatureMonsterModel) {
			std::printf("Loaded creature monster model: %s\n", picked.c_str());
		}
	}
	if (!characters.empty()) {
		characters[0]->isActivePlayer = true;
	}

	std::vector<Character*> playerRefs;
	playerRefs.reserve(characters.size());
	for (auto& c : characters) {
		playerRefs.push_back(c.get());
	}

	std::vector<NPC*> npcRefs;

	NavMesh navMesh;
	CreatureAI creatureAI(&navMesh, nullptr);
	creatureAI.SpawnCreature(glm::vec3(-4.0f, 0.75f, 6.0f), {
		glm::vec3(-8.0f, 0.75f, 6.0f),
		glm::vec3(-2.0f, 0.75f, 6.0f),
		glm::vec3(-2.0f, 0.75f, 2.0f),
		glm::vec3(-8.0f, 0.75f, 2.0f)
	});
	creatureAI.SpawnCreature(glm::vec3(4.0f, 0.75f, 6.0f), {
		glm::vec3(2.0f, 0.75f, 6.0f),
		glm::vec3(8.0f, 0.75f, 6.0f),
		glm::vec3(8.0f, 0.75f, 2.0f),
		glm::vec3(2.0f, 0.75f, 2.0f)
	});
	creatureAI.SpawnCreature(glm::vec3(0.0f, 0.75f, -5.0f), {
		glm::vec3(-3.0f, 0.75f, -4.0f),
		glm::vec3(3.0f, 0.75f, -4.0f),
		glm::vec3(3.0f, 0.75f, -8.0f),
		glm::vec3(-3.0f, 0.75f, -8.0f)
	});
	for (Creature& c : creatureAI.GetCreaturesMutable()) {
		c.humanModel = creatureHumanModel.get();
		c.monsterModel = creatureMonsterModel.get();
	}

	auto modelShader = std::make_unique<Shader>("ModelDebug");
	bool modelShaderReady = false;
	try {
		modelShader->Load("assets/shaders/model_debug.vert", "assets/shaders/model_debug.frag");
		modelShader->Bind();
		modelShader->SetInt("albedoMap", 0);
		modelShader->SetInt("normalMap", 1);
		modelShader->SetInt("roughnessMap", 2);
		modelShader->SetInt("metallicMap", 3);
		modelShader->SetInt("aoMap", 4);
		modelShader->SetInt("emissiveMap", 5);
		modelShader->Unbind();
		modelShaderReady = true;
	} catch (const std::exception& e) {
		std::fprintf(stderr, "Model shader load failed: %s\n", e.what());
	}

	const GLint modelLoc = glGetUniformLocation(characterProgram, "uModel");
	const GLint viewLoc = glGetUniformLocation(characterProgram, "uView");
	const GLint projLoc = glGetUniformLocation(characterProgram, "uProjection");
	const GLint colorLoc = glGetUniformLocation(characterProgram, "uColor");

	Timer timer;
	float aiPrintTimer = 0.0f;
	while (!window.ShouldClose()) {
		timer.Tick();
		const float dt = timer.GetDeltaTime();
		glClearColor(0.05f, 0.03f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const float t = timer.GetTotalTime();

		for (std::size_t i = 0; i < characters.size(); ++i) {
			characters[i]->SetNoiseLevelThisFrame(3.0f + (i == 0 ? 2.0f : 0.0f));
			characters[i]->Update(dt);
		}
		creatureAI.Update(dt, playerRefs, npcRefs);

		const glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 7.0f, 14.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 projection = glm::perspective(glm::radians(60.0f), window.GetAspectRatio(), 0.1f, 100.0f);

		glUseProgram(characterProgram);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glBindVertexArray(cubeVAO);

		for (std::size_t i = 0; i < characters.size(); ++i) {
			const float bob = std::sin(t * 2.0f + static_cast<float>(i)) * 0.12f;
			glm::mat4 model(1.0f);
			model = glm::translate(model, characters[i]->position + glm::vec3(0.0f, bob, 0.0f));
			model = glm::rotate(model, characters[i]->facingAngle + static_cast<float>(i) * 0.15f, glm::vec3(0.0f, 1.0f, 0.0f));

			if (modelShaderReady && characters[i]->model != nullptr) {
				glm::mat4 modelM = model * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
				modelShader->Bind();
				modelShader->SetMat4("model", modelM);
				modelShader->SetMat4("view", view);
				modelShader->SetMat4("projection", projection);
				modelShader->SetVec3("tintColor", characterColors[i]);
				characters[i]->model->Draw(*modelShader);
				modelShader->Unbind();
			} else {
				model = model * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.5f, 0.8f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(colorLoc, 1, glm::value_ptr(characterColors[i]));
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
			}
		}

		auto& creatures = creatureAI.GetCreaturesMutable();
		for (Creature& creature : creatures) {
			glm::mat4 model(1.0f);
			model = glm::translate(model, creature.position + glm::vec3(0.0f, 0.2f, 0.0f));
			model = glm::rotate(model, creature.facingAngle, glm::vec3(0.0f, 1.0f, 0.0f));

			Model* chosenModel = nullptr;
			if (creature.transformProgress >= 0.5f) {
				chosenModel = creature.monsterModel ? creature.monsterModel : creature.humanModel;
			} else {
				chosenModel = creature.humanModel ? creature.humanModel : creature.monsterModel;
			}

			if (modelShaderReady && chosenModel != nullptr) {
				glm::mat4 modelM = model * glm::scale(glm::mat4(1.0f), glm::vec3(1.1f));
				modelShader->Bind();
				modelShader->SetMat4("model", modelM);
				modelShader->SetMat4("view", view);
				modelShader->SetMat4("projection", projection);
				modelShader->SetVec3("tintColor", CreatureStateColor(creature.state));
				chosenModel->Draw(*modelShader);
				modelShader->Unbind();
			} else {
				model = model * glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 1.8f, 0.8f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				const glm::vec3 creatureColor = CreatureStateColor(creature.state);
				glUniform3fv(colorLoc, 1, glm::value_ptr(creatureColor));
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
			}
		}

		glBindVertexArray(0);
		glUseProgram(0);

		aiPrintTimer += dt;
		if (aiPrintTimer >= 1.0f && !creatureAI.GetCreatures().empty()) {
			aiPrintTimer = 0.0f;
			const Creature& c0 = creatureAI.GetCreatures().front();
			const float nearest = creatureAI.GetNearestCreatureDistance(characters.front()->position);
			std::printf("AI Debug | C0 state=%s path=%zu idx=%d nearestToPlayer=%.2f\n",
				CreatureStateName(c0.state),
				c0.currentPath.size(),
				c0.pathIndex,
				nearest);
		}

		window.PollEvents();
		window.SwapBuffers();
	}

	if (cubeEBO) glDeleteBuffers(1, &cubeEBO);
	if (cubeVBO) glDeleteBuffers(1, &cubeVBO);
	if (cubeVAO) glDeleteVertexArrays(1, &cubeVAO);
	if (characterProgram) glDeleteProgram(characterProgram);

	return 0;
}
