#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "engine/renderer/Shader.h"

struct Glyph {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    bool Initialize(const std::string& fontPath = "", unsigned int pixelSize = 48);
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int screenWidth, int screenHeight, bool uppercase = false);
    void Destroy();

    void SetScaleMultiplier(float m) { scaleMultiplier = m; }
    void SetMinimumScale(float m) { minimumScale = m; }

private:
    std::string ResolveFontPath();
    std::unordered_map<char, Glyph> glyphs;
    Shader shader;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    bool ready = false;
    float scaleMultiplier = 1.0f;
    float minimumScale = 0.2f;
};
