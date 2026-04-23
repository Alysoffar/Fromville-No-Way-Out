#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include <glm/gtc/constants.hpp>

#include "Camera.h"
#include "Renderer.h"
#include "Shader.h"

namespace {

struct InstanceParticle {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec4 color = glm::vec4(1.0f);
	float size = 0.05f;
	float rotation = 0.0f;
};

float Random01() {
	return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float RandomRange(float minV, float maxV) {
	return minV + (maxV - minV) * Random01();
}

glm::vec3 RandomSphere(float radius) {
	const float z = RandomRange(-1.0f, 1.0f);
	const float a = RandomRange(0.0f, glm::two_pi<float>());
	const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
	return glm::vec3(std::cos(a) * r, z, std::sin(a) * r) * RandomRange(0.0f, radius);
}

glm::vec3 RandomUnitSphereDirection() {
	const glm::vec3 v = RandomSphere(1.0f);
	const float len = glm::length(v);
	if (len < 0.00001f) {
		return glm::vec3(0.0f, 1.0f, 0.0f);
	}
	return v / len;
}

} // namespace

ParticleSystem::ParticleSystem()
	: particleShader(nullptr), particleTexture(0) {
	particleShader = new Shader("Particle");
	particleShader->Load("assets/shaders/particle.vert", "assets/shaders/particle.frag");

	std::vector<unsigned char> tex(64 * 64 * 4, 0);
	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float u = (static_cast<float>(x) + 0.5f) / 64.0f;
			const float v = (static_cast<float>(y) + 0.5f) / 64.0f;
			const float dx = u - 0.5f;
			const float dy = v - 0.5f;
			const float d = std::sqrt(dx * dx + dy * dy) * 2.0f;
			const float alpha = std::max(0.0f, 1.0f - d);
			const int idx = (y * 64 + x) * 4;
			tex[idx + 0] = 255;
			tex[idx + 1] = 255;
			tex[idx + 2] = 255;
			tex[idx + 3] = static_cast<unsigned char>(alpha * 255.0f);
		}
	}

	glGenTextures(1, &particleTexture);
	glBindTexture(GL_TEXTURE_2D, particleTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

ParticleSystem::~ParticleSystem() {
	for (Emitter& e : emitters) {
		if (e.VAO) {
			glDeleteVertexArrays(1, &e.VAO);
		}
		if (e.VBO) {
			glDeleteBuffers(1, &e.VBO);
		}
		if (e.SSBO) {
			glDeleteBuffers(1, &e.SSBO);
		}
	}

	if (particleTexture) {
		glDeleteTextures(1, &particleTexture);
	}

	delete particleShader;
	particleShader = nullptr;
}

int ParticleSystem::CreateEmitter(EmitterType type, int maxParticles, float emitRate) {
	Emitter e;
	e.type = type;
	e.maxParticles = std::max(1, maxParticles);
	e.emitRate = std::max(0.0f, emitRate);
	e.particles.reserve(static_cast<std::size_t>(e.maxParticles));

	const float quad[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f,
	};

	glGenVertexArrays(1, &e.VAO);
	glBindVertexArray(e.VAO);

	glGenBuffers(1, &e.SSBO);
	glBindBuffer(GL_ARRAY_BUFFER, e.SSBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glGenBuffers(1, &e.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, e.VBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(InstanceParticle) * e.maxParticles), nullptr, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceParticle), (void*)offsetof(InstanceParticle, position));
	glVertexAttribDivisor(1, 1);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceParticle), (void*)offsetof(InstanceParticle, color));
	glVertexAttribDivisor(2, 1);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceParticle), (void*)offsetof(InstanceParticle, size));
	glVertexAttribDivisor(3, 1);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceParticle), (void*)offsetof(InstanceParticle, rotation));
	glVertexAttribDivisor(4, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	emitters.push_back(e);
	return static_cast<int>(emitters.size() - 1);
}

void ParticleSystem::SetEmitterActive(int id, bool active) {
	if (id < 0 || id >= static_cast<int>(emitters.size())) {
		return;
	}
	emitters[static_cast<std::size_t>(id)].active = active;
}

void ParticleSystem::SetEmitterOrigin(int id, glm::vec3 origin) {
	if (id < 0 || id >= static_cast<int>(emitters.size())) {
		return;
	}
	emitters[static_cast<std::size_t>(id)].origin = origin;
}

void ParticleSystem::SetEmitterFollow(int id, glm::vec3* target) {
	if (id < 0 || id >= static_cast<int>(emitters.size())) {
		return;
	}
	emitters[static_cast<std::size_t>(id)].followTarget = target;
}

void ParticleSystem::Burst(glm::vec3 position, int count, EmitterType type) {
	Emitter e;
	e.type = type;
	e.origin = position;
	e.active = true;
	e.burst = true;
	e.maxParticles = std::max(1, count);
	e.emitRate = 0.0f;
	e.particles.reserve(static_cast<std::size_t>(e.maxParticles));

	const int id = CreateEmitter(type, std::max(1, count), 0.0f);
	Emitter& ref = emitters[static_cast<std::size_t>(id)];
	ref.origin = position;
	ref.active = true;
	ref.burst = true;
	for (int i = 0; i < count && static_cast<int>(ref.particles.size()) < ref.maxParticles; ++i) {
		SpawnParticle(ref);
	}
}

void ParticleSystem::Update(float dt) {
	for (Emitter& e : emitters) {
		UpdateEmitter(e, dt);
	}
}

void ParticleSystem::Draw(Renderer& renderer, Camera& camera) {
	(void)renderer;
	if (!particleShader) {
		return;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	particleShader->Bind();
	particleShader->SetMat4("view", camera.GetViewMatrix());
	particleShader->SetMat4("projection", camera.GetProjectionMatrix(16.0f / 9.0f));
	particleShader->SetInt("particleTex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, particleTexture);

	for (Emitter& e : emitters) {
		DrawEmitter(e, camera);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	particleShader->Unbind();
}

void ParticleSystem::SpawnParticle(Emitter& e) {
	if (static_cast<int>(e.particles.size()) >= e.maxParticles) {
		return;
	}
	Particle p;
	ConfigureParticle(p, e.type, e.origin);
	e.particles.push_back(p);
}

void ParticleSystem::ConfigureParticle(Particle& p, EmitterType type, glm::vec3 origin) {
	p.rotation = RandomRange(0.0f, glm::two_pi<float>());
	p.rotationSpeed = RandomRange(-90.0f, 90.0f);

	switch (type) {
	case EmitterType::TORCH_FIRE:
		p.position = origin + RandomSphere(0.1f);
		p.velocity = glm::vec3(RandomRange(-0.1f, 0.1f), RandomRange(0.8f, 1.5f), RandomRange(-0.1f, 0.1f));
		p.color = glm::vec4(1.0f, RandomRange(0.4f, 0.7f), 0.0f, 1.0f);
		p.maxLife = RandomRange(0.5f, 1.0f);
		p.size = RandomRange(0.05f, 0.12f);
		p.rotationSpeed = RandomRange(-90.0f, 90.0f);
		break;
	case EmitterType::BLOOD_CURSE_AURA: {
		const float angle = RandomRange(0.0f, glm::two_pi<float>());
		p.position = origin + glm::vec3(std::cos(angle) * 0.4f, RandomRange(-0.3f, 0.5f), std::sin(angle) * 0.4f);
		p.velocity = glm::vec3(std::cos(angle) * 0.2f, RandomRange(0.1f, 0.4f), std::sin(angle) * 0.2f);
		p.color = glm::vec4(0.7f, 0.0f, 0.0f, 1.0f);
		p.maxLife = RandomRange(0.8f, 1.5f);
		p.size = RandomRange(0.04f, 0.09f);
		break;
	}
	case EmitterType::GLYPH_ACTIVATE: {
		p.position = origin;
		const float angle = RandomRange(0.0f, glm::two_pi<float>());
		const float elev = RandomRange(-glm::pi<float>() * 0.25f, glm::pi<float>() * 0.25f);
		p.velocity = glm::vec3(std::cos(angle) * std::cos(elev), std::sin(elev), std::sin(angle) * std::cos(elev)) * RandomRange(1.5f, 3.5f);
		p.color = glm::vec4(1.0f, 0.85f, 0.1f, 1.0f);
		p.maxLife = RandomRange(0.2f, 0.5f);
		p.size = RandomRange(0.03f, 0.07f);
		break;
	}
	case EmitterType::CREATURE_DISSOLVE:
		p.position = origin + RandomSphere(0.5f);
		p.velocity = RandomUnitSphereDirection() * RandomRange(0.5f, 2.0f);
		p.color = glm::vec4(RandomRange(0.2f, 0.4f), 0.0f, RandomRange(0.3f, 0.5f), 1.0f);
		p.maxLife = RandomRange(0.5f, 1.2f);
		p.size = RandomRange(0.05f, 0.15f);
		break;
	case EmitterType::SPARK:
	default:
		p.position = origin;
		p.velocity = RandomUnitSphereDirection() * RandomRange(1.0f, 4.0f);
		p.color = glm::vec4(1.0f, 0.95f, 0.7f, 1.0f);
		p.maxLife = RandomRange(0.1f, 0.4f);
		p.size = RandomRange(0.02f, 0.05f);
		break;
	}

	p.life = p.maxLife;
}

void ParticleSystem::UpdateEmitter(Emitter& e, float dt) {
	if (e.followTarget) {
		e.origin = *e.followTarget;
	}

	if (e.active && !e.burst) {
		e.emitAccum += e.emitRate * dt;
		while (e.emitAccum >= 1.0f) {
			SpawnParticle(e);
			e.emitAccum -= 1.0f;
		}
	}

	for (Particle& p : e.particles) {
		p.life -= dt;
		if (p.life <= 0.0f) {
			continue;
		}

		p.position += p.velocity * dt;
		p.velocity.y -= 0.5f * dt;
		p.color.a = p.life / std::max(0.0001f, p.maxLife);
		p.size *= 0.998f;
		p.rotation += p.rotationSpeed * dt;
	}

	e.particles.erase(
		std::remove_if(e.particles.begin(), e.particles.end(), [](const Particle& p) { return p.life <= 0.0f; }),
		e.particles.end());

	if (e.burst && e.particles.empty()) {
		e.active = false;
	}
}

void ParticleSystem::DrawEmitter(Emitter& e, Camera& camera) {
	if (e.particles.empty() || !e.VAO || !e.VBO) {
		return;
	}

	std::sort(e.particles.begin(), e.particles.end(), [&](const Particle& a, const Particle& b) {
		const glm::vec3 daVec = a.position - camera.position;
		const glm::vec3 dbVec = b.position - camera.position;
		const float da = glm::dot(daVec, daVec);
		const float db = glm::dot(dbVec, dbVec);
		return da > db;
	});

	UploadToGPU(e);

	glBindVertexArray(e.VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(e.particles.size()));
	glBindVertexArray(0);
}

void ParticleSystem::UploadToGPU(Emitter& e) {
	if (e.particles.empty()) {
		return;
	}

	std::vector<InstanceParticle> instances;
	instances.reserve(e.particles.size());
	for (const Particle& p : e.particles) {
		InstanceParticle i;
		i.position = p.position;
		i.color = p.color;
		i.size = p.size;
		i.rotation = p.rotation;
		instances.push_back(i);
	}

	glBindBuffer(GL_ARRAY_BUFFER, e.VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceParticle)), instances.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

