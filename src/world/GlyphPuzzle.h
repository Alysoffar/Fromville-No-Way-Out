#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

class Camera;
class InputManager;
class ParticleSystem;
class Renderer;

struct GlyphDef {
	int id = -1;
	std::string meaning;
	glm::vec3 worldPosition = glm::vec3(0.0f);
	bool decoded = false;
	int numSegments = 6;
	std::vector<float> segmentTargetAngles;
};

struct PuzzleState {
	GlyphDef* glyph = nullptr;
	std::vector<float> currentAngles;
	int selectedSegment = 0;
	bool solved = false;
	bool active = false;
};

struct GlyphRegistry {
	std::unordered_map<int, std::string> decodedGlyphs;

	void RegisterDecoded(int id, const std::string& meaning);
	bool IsDecoded(int id) const;
	std::string GetMeaning(int id) const;
	std::vector<int> GetAllDecoded() const;
};

class GlyphPuzzle {
public:
	GlyphPuzzle(GlyphRegistry* registry);
	void LoadFromLevel(const nlohmann::json& levelData);
	void Update(float dt, InputManager& input);
	void Draw(Renderer& renderer, Camera& camera);
	void ActivatePuzzle(int glyphId);
	void DeactivatePuzzle();
	bool IsPuzzleActive() const { return state.active; }
	GlyphDef* GetGlyphAt(glm::vec3 pos, float radius = 1.0f);
	void DrawGlyphsInWorld(Renderer& renderer, bool symbolSightActive);

private:
	std::vector<GlyphDef> glyphs;
	PuzzleState state;
	GlyphRegistry* registry;
	float solveCheckTimer = 0.0f;
	ParticleSystem* solveParticles;

	void RenderPuzzleOverlay(Renderer& renderer);
	void CheckSolved();
	bool AllSegmentsAligned() const;
	void OnSolved();
};

