#include "engine/renderer/UIRenderer.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/resources/loader.h"

#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

static unsigned int CompileShaderSource(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[UIRenderer] Shader compilation failed:\n" << infoLog << "\n";
    }
    return shader;
}

static unsigned int LinkProgram(unsigned int vert, unsigned int frag) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[UIRenderer] Shader link failed:\n" << infoLog << "\n";
    }
    return program;
}

bool UIRenderer::Initialize(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    CompileUIShaders();
    CreateQuadMesh();

    return true;
}

void UIRenderer::Shutdown() {
    if (m_quadVAO != 0) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO != 0) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    if (m_buttonVAO != 0) { glDeleteVertexArrays(1, &m_buttonVAO); m_buttonVAO = 0; }
    if (m_buttonVBO != 0) { glDeleteBuffers(1, &m_buttonVBO); m_buttonVBO = 0; }
    if (m_backgroundTexture != 0) { glDeleteTextures(1, &m_backgroundTexture); m_backgroundTexture = 0; }
    if (m_backgroundShader != 0) { glDeleteProgram(m_backgroundShader); m_backgroundShader = 0; }
    if (m_buttonShader != 0) { glDeleteProgram(m_buttonShader); m_buttonShader = 0; }
}

bool UIRenderer::LoadBackground(const std::string& imagePath) {
    if (m_backgroundTexture != 0) {
        glDeleteTextures(1, &m_backgroundTexture);
        m_backgroundTexture = 0;
    }
    return LoadTextureFromFile(imagePath, m_backgroundTexture);
}

void UIRenderer::RenderBackground() {
    if (m_backgroundTexture == 0 || m_backgroundShader == 0 || m_quadVAO == 0) return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_backgroundShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    int bgLoc = glGetUniformLocation(m_backgroundShader, "uBackground");
    if (bgLoc != -1) glUniform1i(bgLoc, 0);

    int darkLoc = glGetUniformLocation(m_backgroundShader, "uDarkness");
    if (darkLoc != -1) glUniform1f(darkLoc, 0.0f); // default bright

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::RenderBackgroundOverlay(float darkness) {
    if (m_backgroundShader == 0 || m_quadVAO == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_backgroundShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    int bgLoc = glGetUniformLocation(m_backgroundShader, "uBackground");
    if (bgLoc != -1) glUniform1i(bgLoc, 0);

    int darkLoc = glGetUniformLocation(m_backgroundShader, "uDarkness");
    if (darkLoc != -1) glUniform1f(darkLoc, darkness);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::AddButton(const UIButton& button) {
    m_buttons.push_back(button);
}

void UIRenderer::ClearButtons() {
    m_buttons.clear();
}

void UIRenderer::RenderButtons() {
    if (m_buttonShader == 0 || m_buttonVAO == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_buttonShader);

    int colorLoc = glGetUniformLocation(m_buttonShader, "uColor");
    int posLoc = glGetUniformLocation(m_buttonShader, "uPosition");
    int sizeLoc = glGetUniformLocation(m_buttonShader, "uSize");

    glBindVertexArray(m_buttonVAO);

    for (const auto& btn : m_buttons) {
        if (!btn.visible) continue;

        glm::vec4 color = btn.hovered ? btn.hoverColor : btn.normalColor;
        if (colorLoc != -1) glUniform4fv(colorLoc, 1, glm::value_ptr(color));

        glm::vec2 ndcPos(btn.x * 2.0f - 1.0f, 1.0f - btn.y * 2.0f);
        if (posLoc != -1) glUniform2fv(posLoc, 1, glm::value_ptr(ndcPos));

        glm::vec2 ndcSize(btn.width, btn.height);
        if (sizeLoc != -1) glUniform2fv(sizeLoc, 1, glm::value_ptr(ndcSize));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    if (m_textRenderer) {
        for (const auto& btn : m_buttons) {
            if (!btn.visible) continue;

            float scale = 0.55f;
            float fontSpacing = 10.0f;
            float textW = static_cast<float>(btn.label.length()) * fontSpacing * scale;
            float screenX = (btn.x * m_screenWidth) - (textW / 2.0f);
            float screenY = m_screenHeight - (btn.y * m_screenHeight) - (12.0f * scale);

            m_textRenderer->RenderText(btn.label, screenX, screenY, scale, glm::vec3(btn.textColor), m_screenWidth, m_screenHeight);
        }
    }
}

void UIRenderer::RenderProgressBar(float cx, float cy,
                                    float width, float height,
                                    float progress,
                                    const glm::vec4& bgColor,
                                    const glm::vec4& fillColor) {
    if (m_buttonShader == 0 || m_buttonVAO == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_buttonShader);

    int colorLoc = glGetUniformLocation(m_buttonShader, "uColor");
    int posLoc = glGetUniformLocation(m_buttonShader, "uPosition");
    int sizeLoc = glGetUniformLocation(m_buttonShader, "uSize");

    glBindVertexArray(m_buttonVAO);

    // 1. Draw background bar
    if (colorLoc != -1) glUniform4fv(colorLoc, 1, glm::value_ptr(bgColor));
    glm::vec2 ndcBgPos(cx * 2.0f - 1.0f, 1.0f - cy * 2.0f);
    if (posLoc != -1) glUniform2fv(posLoc, 1, glm::value_ptr(ndcBgPos));
    glm::vec2 ndcBgSize(width, height);
    if (sizeLoc != -1) glUniform2fv(sizeLoc, 1, glm::value_ptr(ndcBgSize));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Draw fill bar
    if (progress > 0.0f) {
        if (progress > 1.0f) progress = 1.0f;
        if (colorLoc != -1) glUniform4fv(colorLoc, 1, glm::value_ptr(fillColor));

        // Offset center to align left edge
        float fillCx = cx - width * (1.0f - progress);
        glm::vec2 ndcFillPos(fillCx * 2.0f - 1.0f, 1.0f - cy * 2.0f);
        if (posLoc != -1) glUniform2fv(posLoc, 1, glm::value_ptr(ndcFillPos));

        glm::vec2 ndcFillSize(width * progress, height);
        if (sizeLoc != -1) glUniform2fv(sizeLoc, 1, glm::value_ptr(ndcFillSize));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::UpdateMouseInput(double mouseX, double mouseY, bool clicked) {
    static bool wasClicked = false;
    for (auto& btn : m_buttons) {
        if (!btn.visible) continue;
        btn.hovered = IsMouseOverButton(btn, mouseX, mouseY);
        if (btn.hovered && clicked && !wasClicked) {
            if (btn.onClick) {
                btn.onClick();
            }
        }
    }
    wasClicked = clicked;
}

void UIRenderer::RenderText(const std::string& text, float x, float y,
                            float scale, const glm::vec4& color) {
    if (!m_textRenderer) return;

    float textW = static_cast<float>(text.length()) * 10.0f * scale;
    float screenX = (x * m_screenWidth) - (textW / 2.0f);
    float screenY = m_screenHeight - (y * m_screenHeight) - (12.0f * scale);

    m_textRenderer->RenderText(text, screenX, screenY, scale, glm::vec3(color), m_screenWidth, m_screenHeight);
}

void UIRenderer::RenderTitle(const std::string& text, float x, float y,
                             float scale, const glm::vec4& color) {
    if (!m_textRenderer) return;

    float textW = static_cast<float>(text.length()) * 14.0f * scale;
    float screenX = (x * m_screenWidth) - (textW / 2.0f);
    float screenY = m_screenHeight - (y * m_screenHeight) - (12.0f * scale);

    m_textRenderer->RenderText(text, screenX, screenY, scale, glm::vec3(color), m_screenWidth, m_screenHeight);
}

void UIRenderer::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

bool UIRenderer::LoadTextureFromFile(const std::string& path, unsigned int& textureID) {
    int width = 0, height = 0;
    std::vector<unsigned char> pixels;
    if (!Loader::LoadImageRGBA(path, width, height, pixels)) {
        std::cerr << "[UIRenderer] Failed to load image: " << path << "\n";
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void UIRenderer::CreateQuadMesh() {
    float quadVertices[] = {
        // pos (NDC)     // uv
        -1.0f,  1.0f,   0.0f, 0.0f,
        -1.0f, -1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 1.0f,
         1.0f,  1.0f,   1.0f, 0.0f
    };
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    float buttonVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };
    glGenVertexArrays(1, &m_buttonVAO);
    glGenBuffers(1, &m_buttonVBO);
    glBindVertexArray(m_buttonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_buttonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), buttonVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void UIRenderer::CompileUIShaders() {
    std::string vertSrc = Loader::ReadTextFile("shaders/ui_background.vert");
    std::string fragSrc = Loader::ReadTextFile("shaders/ui_background.frag");
    if (!vertSrc.empty() && !fragSrc.empty()) {
        unsigned int vert = CompileShaderSource(GL_VERTEX_SHADER, vertSrc);
        unsigned int frag = CompileShaderSource(GL_FRAGMENT_SHADER, fragSrc);
        m_backgroundShader = LinkProgram(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    vertSrc = Loader::ReadTextFile("shaders/ui_button.vert");
    fragSrc = Loader::ReadTextFile("shaders/ui_button.frag");
    if (!vertSrc.empty() && !fragSrc.empty()) {
        unsigned int vert = CompileShaderSource(GL_VERTEX_SHADER, vertSrc);
        unsigned int frag = CompileShaderSource(GL_FRAGMENT_SHADER, fragSrc);
        m_buttonShader = LinkProgram(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
    }
}

bool UIRenderer::IsMouseOverButton(const UIButton& btn, double mouseX, double mouseY) {
    float normX = static_cast<float>(mouseX) / m_screenWidth;
    float normY = static_cast<float>(mouseY) / m_screenHeight;

    return (normX >= btn.x - btn.width / 2.0f && normX <= btn.x + btn.width / 2.0f &&
            normY >= btn.y - btn.height / 2.0f && normY <= btn.y + btn.height / 2.0f);
}
