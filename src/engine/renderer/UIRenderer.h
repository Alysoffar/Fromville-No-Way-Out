#pragma once
#include <string>
#include <functional>
#include <vector>
#include <glm/glm.hpp>

class TextRenderer;

struct UIButton {
    std::string label;
    float x, y;           // center position in normalized coords (0-1)
    float width, height;  // in normalized coords
    glm::vec4 normalColor;
    glm::vec4 hoverColor;
    glm::vec4 textColor;
    std::function<void()> onClick;
    bool hovered = false;
    bool visible = true;
};

class UIRenderer {
public:
    bool Initialize(int screenWidth, int screenHeight);
    void Shutdown();

    // Background image
    bool LoadBackground(const std::string& imagePath);
    void RenderBackground();
    void RenderBackgroundOverlay(float darkness);

    // Buttons
    void AddButton(const UIButton& button);
    void ClearButtons();
    void RenderButtons();
    void UpdateMouseInput(double mouseX, double mouseY, bool clicked);
    std::vector<UIButton>& GetButtons() { return m_buttons; }
    const std::vector<UIButton>& GetButtons() const { return m_buttons; }


    // Progress Bar
    void RenderProgressBar(float cx, float cy,
                           float width, float height,
                           float progress,
                           const glm::vec4& bgColor,
                           const glm::vec4& fillColor);

    // Text
    void RenderText(const std::string& text, float x, float y,
                    float scale, const glm::vec4& color);

    // Title text (larger)
    void RenderTitle(const std::string& text, float x, float y,
                     float scale, const glm::vec4& color);

    void SetScreenSize(int width, int height);
    void SetTextRenderer(TextRenderer* textRenderer) { m_textRenderer = textRenderer; }

private:
    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    // Background quad
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    unsigned int m_backgroundTexture = 0;
    unsigned int m_backgroundShader = 0;

    // Button rendering
    unsigned int m_buttonShader = 0;
    unsigned int m_buttonVAO = 0;
    unsigned int m_buttonVBO = 0;

    std::vector<UIButton> m_buttons;

    // Pointer to existing TextRenderer
    TextRenderer* m_textRenderer = nullptr;

    bool LoadTextureFromFile(const std::string& path, unsigned int& textureID);
    void CreateQuadMesh();
    void CompileUIShaders();
    bool IsMouseOverButton(const UIButton& btn, double mouseX, double mouseY);
};
