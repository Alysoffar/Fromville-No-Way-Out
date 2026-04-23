#include "GlyphPuzzle.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>

#include "core/InputManager.h"
#include "renderer/Camera.h"
#include "renderer/Renderer.h"

namespace {

constexpr float kSegmentRotateSpeed = 45.0f;

float WrapAngle(float a) {
	while (a < 0.0f) {
		a += 360.0f;
	}
	while (a >= 360.0f) {
		a -= 360.0f;
	}
	return a;
}

float RandomOffsetDegrees() {
	return static_cast<float>(std::rand() % 360);
}

void EmitGlyphDecodedEvent(int glyphId, const std::string& meaning) {
	std::printf("Event: GLYPH_DECODED id=%d meaning=%s\n", glyphId, meaning.c_str());
}

void SetPlayerControllerPuzzleLock(bool lockInput) {
	(void)lockInput;
	// Hook: integrate with PlayerController to freeze non-puzzle movement/actions.
}

void PlayGlyphSolveAudio(const glm::vec3& position) {
	(void)position;
	std::printf("Audio: glyph_solve.ogg\n");
}

void BurstGlyphParticles(ParticleSystem* system, const glm::vec3& position, int count) {
	(void)system;
	(void)position;
	(void)count;
	// Hook: solveParticles->Burst(position, count, GLYPH_ACTIVATE)
}

void DrawGlyphDecal(Renderer& renderer, const GlyphDef& glyph, const glm::vec3& emissionColor) {
	(void)renderer;
	(void)glyph;
	(void)emissionColor;
	// Hook: render flat decal aligned to nearest wall and emit color.
}

} // namespace

void GlyphRegistry::RegisterDecoded(int id, const std::string& meaning) {
	decodedGlyphs[id] = meaning;
}

bool GlyphRegistry::IsDecoded(int id) const {
	return decodedGlyphs.find(id) != decodedGlyphs.end();
}

std::string GlyphRegistry::GetMeaning(int id) const {
	auto it = decodedGlyphs.find(id);
	if (it == decodedGlyphs.end()) {
		return std::string();
	}
	return it->second;
}

std::vector<int> GlyphRegistry::GetAllDecoded() const {
	std::vector<int> ids;
	ids.reserve(decodedGlyphs.size());
	for (const auto& kv : decodedGlyphs) {
		ids.push_back(kv.first);
	}
	return ids;
}

GlyphPuzzle::GlyphPuzzle(GlyphRegistry* registryPtr)
	: registry(registryPtr), solveParticles(nullptr) {}

void GlyphPuzzle::LoadFromLevel(const nlohmann::json& levelData) {
	glyphs.clear();

	const auto it = levelData.find("glyphs");
	if (it == levelData.end() || !it->is_array()) {
		return;
	}

	for (const auto& g : *it) {
		GlyphDef glyph;
		glyph.id = g.value("id", static_cast<int>(glyphs.size()));
		glyph.meaning = g.value("meaning", std::string());
		glyph.decoded = g.value("decoded", false);
		glyph.numSegments = std::clamp(g.value("numSegments", 6), 6, 8);
		if (g.contains("worldPosition") && g["worldPosition"].is_array() && g["worldPosition"].size() >= 3) {
			glyph.worldPosition = glm::vec3(
				g["worldPosition"][0].get<float>(),
				g["worldPosition"][1].get<float>(),
				g["worldPosition"][2].get<float>());
		}

		glyph.segmentTargetAngles.clear();
		if (g.contains("segmentTargetAngles") && g["segmentTargetAngles"].is_array()) {
			for (const auto& angle : g["segmentTargetAngles"]) {
				glyph.segmentTargetAngles.push_back(WrapAngle(angle.get<float>()));
			}
		}
		if (static_cast<int>(glyph.segmentTargetAngles.size()) < glyph.numSegments) {
			glyph.segmentTargetAngles.resize(glyph.numSegments, 0.0f);
		}

		glyphs.push_back(std::move(glyph));
	}
}

void GlyphPuzzle::Update(float dt, InputManager& input) {
	if (!state.active || !state.glyph) {
		return;
	}

	if (input.IsKeyDown(GLFW_KEY_LEFT) || input.IsKeyDown(GLFW_KEY_A)) {
		state.currentAngles[state.selectedSegment] = WrapAngle(state.currentAngles[state.selectedSegment] - kSegmentRotateSpeed * dt);
	}
	if (input.IsKeyDown(GLFW_KEY_RIGHT) || input.IsKeyDown(GLFW_KEY_D)) {
		state.currentAngles[state.selectedSegment] = WrapAngle(state.currentAngles[state.selectedSegment] + kSegmentRotateSpeed * dt);
	}

	if (input.IsKeyPressed(GLFW_KEY_TAB) || input.IsKeyPressed(GLFW_KEY_Q)) {
		state.selectedSegment = (state.selectedSegment + 1) % std::max(1, state.glyph->numSegments);
	}

	if (input.IsKeyPressed(GLFW_KEY_ESCAPE)) {
		DeactivatePuzzle();
		return;
	}

	solveCheckTimer -= dt;
	if (solveCheckTimer <= 0.0f) {
		CheckSolved();
		solveCheckTimer = 0.1f;
	}
}

void GlyphPuzzle::Draw(Renderer& renderer, Camera& camera) {
	(void)camera;
	if (state.active) {
		RenderPuzzleOverlay(renderer);
	}
}

void GlyphPuzzle::ActivatePuzzle(int glyphId) {
	auto it = std::find_if(glyphs.begin(), glyphs.end(), [glyphId](const GlyphDef& g) { return g.id == glyphId; });
	if (it == glyphs.end()) {
		return;
	}

	state.glyph = &(*it);
	state.currentAngles.assign(state.glyph->numSegments, 0.0f);
	for (float& a : state.currentAngles) {
		a = RandomOffsetDegrees();
	}
	state.selectedSegment = 0;
	state.solved = false;
	state.active = true;
	solveCheckTimer = 0.1f;

	SetPlayerControllerPuzzleLock(true);
}

void GlyphPuzzle::DeactivatePuzzle() {
	state.active = false;
	state.glyph = nullptr;
	state.currentAngles.clear();
	state.selectedSegment = 0;

	SetPlayerControllerPuzzleLock(false);
}

GlyphDef* GlyphPuzzle::GetGlyphAt(glm::vec3 pos, float radius) {
	const float r2 = radius * radius;
	for (GlyphDef& g : glyphs) {
		const glm::vec3 d = g.worldPosition - pos;
		if (glm::dot(d, d) <= r2) {
			return &g;
		}
	}
	return nullptr;
}

void GlyphPuzzle::DrawGlyphsInWorld(Renderer& renderer, bool symbolSightActive) {
	const float t = static_cast<float>(std::clock()) / static_cast<float>(CLOCKS_PER_SEC);
	const float pulse = 0.5f + 0.5f * std::sin(t * 2.0f);

	for (const GlyphDef& g : glyphs) {
		glm::vec3 emission(0.0f);
		if (g.decoded) {
			emission = glm::vec3(1.0f, 0.85f, 0.2f);
		} else if (symbolSightActive) {
			emission = glm::vec3(0.4f, 0.6f, 1.0f) * (0.6f + 0.4f * pulse);
		}
		DrawGlyphDecal(renderer, g, emission);
	}
}

void GlyphPuzzle::RenderPuzzleOverlay(Renderer& renderer) {
	(void)renderer;
	// Hook: render ring segments and selected segment highlight.
}

void GlyphPuzzle::CheckSolved() {
	if (!state.active || !state.glyph || state.solved) {
		return;
	}

	if (AllSegmentsAligned()) {
		OnSolved();
	}
}

bool GlyphPuzzle::AllSegmentsAligned() const {
	if (!state.glyph) {
		return false;
	}

	const int count = std::min(static_cast<int>(state.currentAngles.size()), static_cast<int>(state.glyph->segmentTargetAngles.size()));
	for (int i = 0; i < count; ++i) {
		const float current = WrapAngle(state.currentAngles[i]);
		const float target = WrapAngle(state.glyph->segmentTargetAngles[i]);
		float diff = std::fabs(current - target);
		diff = std::min(diff, 360.0f - diff);
		if (diff > 3.0f) {
			return false;
		}
	}
	return count > 0;
}

void GlyphPuzzle::OnSolved() {
	if (!state.glyph) {
		return;
	}

	state.solved = true;
	state.glyph->decoded = true;

	if (registry) {
		registry->RegisterDecoded(state.glyph->id, state.glyph->meaning);
	}

	BurstGlyphParticles(solveParticles, state.glyph->worldPosition, 200);
	PlayGlyphSolveAudio(state.glyph->worldPosition);
	EmitGlyphDecodedEvent(state.glyph->id, state.glyph->meaning);

	DeactivatePuzzle();
}

