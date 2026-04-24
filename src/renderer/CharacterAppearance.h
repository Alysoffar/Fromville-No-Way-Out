#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

struct CharacterTextures {
    GLuint albedo = 0;
    GLuint normal = 0;
    GLuint roughness = 0;
    GLuint metallic = 0;
    GLuint ao = 0;
    GLuint emissive = 0;
    GLuint subsurface = 0;
    GLuint dirt = 0;
    GLuint eyeAlbedo = 0;
    GLuint eyeNormal = 0;
};

struct CharacterMaterial {
    CharacterTextures textures;
    glm::vec3 skinTint = glm::vec3(1.0f);
    glm::vec3 hairColor = glm::vec3(0.1f, 0.08f, 0.06f);
    glm::vec3 clothingTint = glm::vec3(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float emissiveStrength = 0.0f;
    float subsurfaceStrength = 0.6f;
    float dirtAmount = 0.0f;
    float wetness = 0.0f;
    bool useSubsurface = true;
    bool useEyeShader = true;
};

struct CharacterAppearance {
    std::string characterName;
    CharacterMaterial bodyMaterial;
    CharacterMaterial clothingMaterial;
    CharacterMaterial hairMaterial;
    float height = 1.78f;
    glm::vec3 modelScale = glm::vec3(1.0f);
    std::string modelPath;
    std::vector<std::string> animationPaths;
};

class CharacterMaterialSystem {
public:
    ~CharacterMaterialSystem();

    void LoadAppearance(const std::string& characterName, CharacterAppearance& out);
    void BindMaterial(const CharacterMaterial& mat, Shader& shader, int textureUnitStart);
    GLuint LoadTexture(const std::string& path, bool sRGB);
    GLuint GenerateFallbackTexture(glm::vec4 color);

private:
    std::unordered_map<std::string, GLuint> textureCache;
    std::vector<GLuint> ownedTextures;
    GLuint fallbackWhite = 0;
    GLuint fallbackNormal = 0;
    GLuint fallbackBlack = 0;

    GLuint EnsureFallbackWhite();
    GLuint EnsureFallbackNormal();
    GLuint EnsureFallbackBlack();
};
