#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Renderer;
class Shader;

enum class EmitterType {
	TORCH_FIRE,
	BLOOD_CURSE_AURA,
	GLYPH_ACTIVATE,
	CREATURE_DISSOLVE,
	SPARK
};

struct Particle {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	glm::vec4 color = glm::vec4(1.0f);
	float life = 0.0f;
	float maxLife = 0.0f;
	float size = 0.05f;
	float rotation = 0.0f;
	float rotationSpeed = 0.0f;
};

struct Emitter {
	EmitterType type = EmitterType::SPARK;
	glm::vec3 origin = glm::vec3(0.0f);
	glm::vec3* followTarget = nullptr;
	bool active = false;
	bool burst = false;
	int maxParticles = 0;
	float emitRate = 0.0f;
	float emitAccum = 0.0f;
	std::vector<Particle> particles;
	GLuint SSBO = 0;
	GLuint VAO = 0;
	GLuint VBO = 0;
};

class ParticleSystem {
public:
	ParticleSystem();
	~ParticleSystem();
	int CreateEmitter(EmitterType type, int maxParticles, float emitRate);
	void SetEmitterActive(int id, bool active);
	void SetEmitterOrigin(int id, glm::vec3 origin);
	void SetEmitterFollow(int id, glm::vec3* target);
	void Burst(glm::vec3 position, int count, EmitterType type);
	void Update(float dt);
	void Draw(Renderer& renderer, Camera& camera);

private:
	std::vector<Emitter> emitters;
	Shader* particleShader;
	GLuint particleTexture;

	void SpawnParticle(Emitter& e);
	void ConfigureParticle(Particle& p, EmitterType type, glm::vec3 origin);
	void UpdateEmitter(Emitter& e, float dt);
	void DrawEmitter(Emitter& e, Camera& camera);
	void UploadToGPU(Emitter& e);
};

