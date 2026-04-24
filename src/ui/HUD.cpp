#include "HUD.h"
#include "renderer/Shader.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

HUD::HUD() {}

HUD::~HUD() {
    if (ftFace) FT_Done_Face(ftFace);
    if (ftLib) FT_Done_FreeType(ftLib);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (hudShader) delete hudShader;
    if (colorShader) delete colorShader;
}

void HUD::Init(Renderer* renderer) {
    this->renderer = renderer;

    if (FT_Init_FreeType(&ftLib)) {
        std::cerr << "HUD: Could not init FreeType Library" << std::endl;
        return;
    }

    if (FT_New_Face(ftLib, "assets/fonts/arial.ttf", 0, &ftFace)) {
        std::cerr << "HUD: Failed to load font" << std::endl;
        return;
    }
    
    FT_Set_Pixel_Sizes(ftFace, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(ftFace, c, FT_LOAD_RENDER)) {
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            ftFace->glyph->bitmap.width,
            ftFace->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ftFace->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GlyphTexture character = {
            texture,
            glm::ivec2(ftFace->glyph->bitmap.width, ftFace->glyph->bitmap.rows),
            glm::ivec2(ftFace->glyph->bitmap_left, ftFace->glyph->bitmap_top),
            static_cast<unsigned int>(ftFace->glyph->advance.x)
        };
        glyphTextures.insert(std::pair<char, GlyphTexture>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    hudShader = new Shader("HUD");
    try {
        hudShader->Load("assets/shaders/text.vert", "assets/shaders/text.frag");
    } catch(...) {
        std::cerr << "HUD: Failed to load text shader" << std::endl;
    }

    colorShader = new Shader("Color");
    try {
        colorShader->Load("assets/shaders/color.vert", "assets/shaders/color.frag");
    } catch(...) {
        std::cerr << "HUD: Failed to load color shader" << std::endl;
    }
}

void HUD::Update(float deltaTime, HUDState state) {
    this->dt = deltaTime;
    this->currentState = state;

    for (auto it = notifications.begin(); it != notifications.end();) {
        it->remaining -= dt;
        if (it->remaining <= 0.0f) {
            it = notifications.erase(it);
        } else {
            ++it;
        }
    }
}

void HUD::ShowNotification(const std::string& text, float duration) {
    notifications.push_back({text, duration, duration});
    if (notifications.size() > 3) {
        notifications.erase(notifications.begin());
    }
}

void HUD::Draw() {
    if (currentState.puzzleActive || currentState.dialogueActive) {
        return; // Suppress normal HUD during puzzles/dialogues
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    if (hudShader) {
        hudShader->Bind();
        // Assume 1280x720 for now
        glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);
        hudShader->SetMat4("projection", projection);

        DrawHealthBreathEffect();
        DrawInteractPrompt();
        
        if (currentState.showSwitchMenu) {
            DrawCharacterSwitchMenu();
        }

        DrawNotifications();

        hudShader->Unbind();
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void HUD::DrawDeathScreen(const std::string& characterName) {
    if (!renderer || !hudShader || !colorShader) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);

    // 1. Draw full-screen dark overlay
    colorShader->Bind();
    colorShader->SetMat4("projection", projection);
    colorShader->SetVec4("color", glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    
    float quadVertices[6][4] = {
        { 0.0f,    720.0f, 0.0f, 0.0f },
        { 0.0f,    0.0f,   0.0f, 0.0f },
        { 1280.0f, 0.0f,   0.0f, 0.0f },
        { 0.0f,    720.0f, 0.0f, 0.0f },
        { 1280.0f, 0.0f,   0.0f, 0.0f },
        { 1280.0f, 720.0f, 0.0f, 0.0f }
    };
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    colorShader->Unbind();

    // 2. Draw Text
    hudShader->Bind();
    hudShader->SetMat4("projection", projection);
    
    float centerX = 1280.0f / 2.0f;
    float centerY = 720.0f / 2.0f;

    std::string title = "YOU DIED";
    std::string nameLine = characterName;
    std::string prompt = "Press R to restart  |  Press Esc to quit";
    
    // Scale 2.0 dark red
    DrawText(title, centerX - (title.length() * 20.0f), centerY + 100.0f, 2.0f, glm::vec3(0.7f, 0.05f, 0.05f));
    // Scale 0.8 white
    DrawText(nameLine, centerX - (nameLine.length() * 8.0f), centerY, 0.8f, glm::vec3(1.0f));
    // Scale 0.6 grey
    DrawText(prompt, centerX - (prompt.length() * 6.0f), centerY - 100.0f, 0.6f, glm::vec3(0.6f, 0.6f, 0.6f));
    
    hudShader->Unbind();
    glEnable(GL_DEPTH_TEST);
}

void HUD::DrawHealthBreathEffect() {
    // Handled by post-processing in renderer
}

void HUD::DrawInteractPrompt() {
    if (!currentState.interactHint.empty()) {
        DrawText(currentState.interactHint, 640.0f - (currentState.interactHint.length() * 5.0f), 360.0f - 50.0f, 0.5f, glm::vec3(0.8f, 0.8f, 0.8f));
    }
    if (!currentState.activeAbilityHint.empty()) {
        DrawText(currentState.activeAbilityHint, 1280.0f - 250.0f, 50.0f, 0.4f, glm::vec3(0.6f, 0.8f, 1.0f));
    }
}

void HUD::DrawCharacterSwitchMenu() {
    float startX = 640.0f - (currentState.characterPortraits.size() * 100.0f) / 2.0f;
    for (size_t i = 0; i < currentState.characterPortraits.size(); ++i) {
        auto& p = currentState.characterPortraits[i];
        glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
        if (p.second < 50.0f) color = glm::vec3(1.0f, 1.0f, 0.0f);
        if (p.second < 20.0f) color = glm::vec3(1.0f, 0.0f, 0.0f);
        
        if (p.first == currentState.characterName) {
            DrawText("[" + p.first + "]", startX + i * 100.0f, 100.0f, 0.5f, glm::vec3(1.0f));
        } else {
            DrawText(p.first, startX + i * 100.0f, 100.0f, 0.4f, color);
        }
    }
}

void HUD::DrawNotifications() {
    float y = 650.0f;
    for (const auto& notif : notifications) {
        float alpha = 1.0f;
        if (notif.remaining < 0.3f) alpha = notif.remaining / 0.3f;
        else if (notif.maxDuration - notif.remaining < 0.2f) alpha = (notif.maxDuration - notif.remaining) / 0.2f;
        
        DrawText(notif.text, 640.0f - (notif.text.length() * 5.0f), y, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f) * alpha);
        y -= 30.0f;
    }
}

void HUD::DrawText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    if (!hudShader) return;
    hudShader->SetVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(quadVAO);

    for (std::string::const_iterator c = text.begin(); c != text.end(); c++) {
        GlyphTexture ch = glyphTextures[*c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HUD::RenderCharacter(char c, float x, float y, float scale) {
}
