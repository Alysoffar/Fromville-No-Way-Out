#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

class Renderer;
class Shader;

struct HUDState {
    float health = 100.0f;
    float maxHealth = 100.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    
    // Character-specific
    float curseMeter = 0.0f;           // Boyd only
    float redemptionScore = 0.0f;      // Sara only
    bool  symbolSightActive = false;   // Jade only
    bool  isGhostStep = false;         // Sara only
    bool  memoryModeActive = false;    // Victor only
    float breathHoldRemaining = 0.0f;  // Victor only
    
    std::string characterName;
    std::string activeAbilityHint;  // "Q: Interrogate [3m]" etc
    std::string interactHint;       // "E: Examine glyph" etc
    
    bool puzzleActive = false;
    bool dialogueActive = false;
    bool showSwitchMenu = false;
    
    // name, health%
    std::vector<std::pair<std::string, float>> characterPortraits;
};

struct GlyphTexture {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class HUD {
public:
    HUD();
    ~HUD();
    
    void Init(Renderer* renderer);
    void Update(float dt, HUDState state);
    void Draw();
    void DrawDeathScreen(const std::string& characterName);
    void ShowNotification(const std::string& text, float duration = 3.0f);

private:
    struct Notification { 
        std::string text; 
        float remaining; 
        float maxDuration;
    };
    
    std::vector<Notification> notifications;
    HUDState currentState;
    float dt = 0.0f;
    
    Renderer* renderer = nullptr;
    Shader* hudShader = nullptr;
    Shader* colorShader = nullptr;
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    
    FT_Library ftLib = nullptr;
    FT_Face    ftFace = nullptr;
    std::unordered_map<char, GlyphTexture> glyphTextures;

    void DrawHealthBreathEffect();    // vignette pulse when health low
    void DrawInteractPrompt();        // floating text near crosshair
    void DrawCharacterSwitchMenu();   // portrait grid during Tab hold
    void DrawNotifications();         // timed text popups
    
    void DrawText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    void RenderCharacter(char c, float x, float y, float scale);
};
