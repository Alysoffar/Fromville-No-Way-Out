#include "engine/renderer/TextRenderer.h"

#include <array>
#include <filesystem>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {
std::string ToUpperAscii(std::string value) {
	for (char& ch : value) {
		if (ch >= 'a' && ch <= 'z') {
			ch = static_cast<char>(ch - 'a' + 'A');
		}
	}
	return value;
}
}

TextRenderer::TextRenderer()
	: shader("HUDText"), whiteTextureID(0) {
}

TextRenderer::~TextRenderer() {
	Destroy();
}

void TextRenderer::Destroy() {
	for (auto& [character, glyph] : glyphs) {
		(void)character;
		if (glyph.textureID != 0) {
			glDeleteTextures(1, &glyph.textureID);
			glyph.textureID = 0;
		}
	}
	glyphs.clear();

	if (whiteTextureID != 0) {
		glDeleteTextures(1, &whiteTextureID);
		whiteTextureID = 0;
	}

	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	ready = false;
}

std::string TextRenderer::ResolveFontPath() {
	const std::array<std::filesystem::path, 4> candidates = {
		std::filesystem::path("C:/Windows/Fonts/georgia.ttf"),
		std::filesystem::path("C:/Windows/Fonts/times.ttf"),
		std::filesystem::path("C:/Windows/Fonts/segoeui.ttf"),
		std::filesystem::path("C:/Windows/Fonts/arial.ttf")
	};

	for (const auto& candidate : candidates) {
		if (std::filesystem::exists(candidate)) {
			return candidate.string();
		}
	}

	return {};
}

bool TextRenderer::Initialize(const std::string& fontPath, unsigned int pixelSize) {
	Destroy();

	FT_Library library = nullptr;
	if (FT_Init_FreeType(&library) != 0) {
		std::cerr << "[HUD] Failed to init FreeType\n";
		return false;
	}

	FT_Face face = nullptr;
	const std::string resolvedFontPath = fontPath.empty() ? ResolveFontPath() : fontPath;
	if (resolvedFontPath.empty() || FT_New_Face(library, resolvedFontPath.c_str(), 0, &face) != 0) {
		std::cerr << "[HUD] Failed to load font face\n";
		FT_Done_FreeType(library);
		return false;
	}

	FT_Set_Pixel_Sizes(face, 0, pixelSize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	shader.Load("assets/shaders/text.vert", "assets/shaders/text.frag");

	for (unsigned char c = 32; c < 127; ++c) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
			continue;
		}

		unsigned int texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Glyph glyph;
		glyph.textureID = texture;
		glyph.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		glyph.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
		glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);
		glyphs.emplace(static_cast<char>(c), glyph);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Generate a 1x1 solid white texture for rendering solid background planes
	unsigned char whitePixel[] = { 255 };
	glGenTextures(1, &whiteTextureID);
	glBindTexture(GL_TEXTURE_2D, whiteTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	ready = !glyphs.empty() && vao != 0 && vbo != 0 && whiteTextureID != 0;
	if (!ready) {
		Destroy();
		std::cerr << "[HUD] Text renderer not ready\n";
	}

	return ready;
}

void TextRenderer::RenderText(const std::string& inputText, float x, float y, float scale, glm::vec3 color, int screenWidth, int screenHeight, float alpha) {
	if (!ready || inputText.empty()) {
		return;
	}

	scale = std::max(scale, minimumScale) * scaleMultiplier;
	std::string text = ToUpperAscii(inputText);
	const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	shader.Bind();
	shader.SetMat4("projection", projection);
	shader.SetVec3("textColor", color);
	shader.SetFloat("textAlpha", alpha);
	shader.SetInt("text", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(vao);

	const float lineHeight = static_cast<float>(screenHeight) * 0.032f;
	float cursorY = y;
	for (char ch : text) {
		if (ch == '\n') {
			cursorY -= lineHeight;
			x = 24.0f;
			continue;
		}

		const auto found = glyphs.find(ch);
		if (found == glyphs.end()) {
			x += 10.0f * scale;
			continue;
		}

		const Glyph& glyph = found->second;
		const float xpos = x + static_cast<float>(glyph.bearing.x) * scale;
		const float ypos = cursorY - static_cast<float>(glyph.size.y - glyph.bearing.y) * scale;
		const float w = static_cast<float>(glyph.size.x) * scale;
		const float h = static_cast<float>(glyph.size.y) * scale;

		const float vertices[6][4] = {
			{xpos,     ypos + h, 0.0f, 0.0f},
			{xpos,     ypos,     0.0f, 1.0f},
			{xpos + w, ypos,     1.0f, 1.0f},
			{xpos,     ypos + h, 0.0f, 0.0f},
			{xpos + w, ypos,     1.0f, 1.0f},
			{xpos + w, ypos + h, 1.0f, 0.0f}
		};

		glBindTexture(GL_TEXTURE_2D, glyph.textureID);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += static_cast<float>(glyph.advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	shader.Unbind();
	glEnable(GL_DEPTH_TEST);
}

void TextRenderer::RenderQuad(float x, float y, float w, float h, glm::vec3 color, int screenWidth, int screenHeight) {
	if (!ready || whiteTextureID == 0) {
		return;
	}

	const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	shader.Bind();
	shader.SetMat4("projection", projection);
	shader.SetVec3("textColor", color);
	shader.SetInt("text", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, whiteTextureID);
	glBindVertexArray(vao);

	const float vertices[6][4] = {
		{x,     y + h, 0.0f, 0.0f},
		{x,     y,     0.0f, 1.0f},
		{x + w, y,     1.0f, 1.0f},
		{x,     y + h, 0.0f, 0.0f},
		{x + w, y,     1.0f, 1.0f},
		{x + w, y + h, 1.0f, 0.0f}
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	shader.Unbind();
	glEnable(GL_DEPTH_TEST);
}
