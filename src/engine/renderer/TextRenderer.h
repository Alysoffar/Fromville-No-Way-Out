#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "engine/renderer/Shader.h"

class TextRenderer {
public:
	TextRenderer();
	~TextRenderer();

	bool Initialize(const std::string& fontPath, unsigned int pixelSize = 30);
	void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int screenWidth, int screenHeight);
	void SetScaleMultiplier(float multiplier) { scaleMultiplier = multiplier; }
	float GetScaleMultiplier() const { return scaleMultiplier; }
	void SetMinimumScale(float minimum) { minimumScale = minimum; }
	float GetMinimumScale() const { return minimumScale; }

	bool IsReady() const { return ready; }

private:
	struct Glyph {
		unsigned int textureID = 0;
		glm::ivec2 size = glm::ivec2(0);
		glm::ivec2 bearing = glm::ivec2(0);
		unsigned int advance = 0;
	};

	Shader shader;
	unsigned int vao = 0;
	unsigned int vbo = 0;
	std::unordered_map<char, Glyph> glyphs;
	bool ready = false;
	float scaleMultiplier = 1.0f;
	float minimumScale = 0.0f;

	void Destroy();
	static std::string ResolveFontPath();
};
