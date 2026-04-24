#include "CharacterAppearance.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

#include "Shader.h"
#include <stb_image.h>

namespace {

std::string ToLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string NormalizeCharacterName(const std::string& in) {
    std::string n = ToLower(in);
    n.erase(std::remove_if(n.begin(), n.end(), [](char c) { return c == ' ' || c == '_' || c == '-'; }), n.end());
    return n;
}

} // namespace

CharacterMaterialSystem::~CharacterMaterialSystem() {
    if (!ownedTextures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(ownedTextures.size()), ownedTextures.data());
    }
}

void CharacterMaterialSystem::LoadAppearance(const std::string& characterName, CharacterAppearance& out) {
    const std::string key = NormalizeCharacterName(characterName);
    out = CharacterAppearance{};

    auto loadAll = [this](const std::string& base, CharacterMaterial& m) {
        m.textures.albedo = LoadTexture(base + "albedo.png", true);
        m.textures.normal = LoadTexture(base + "normal.png", false);
        m.textures.roughness = LoadTexture(base + "roughness.png", false);
        m.textures.metallic = LoadTexture(base + "metallic.png", false);
        m.textures.ao = LoadTexture(base + "ao.png", false);
        m.textures.emissive = LoadTexture(base + "emissive.png", true);
        m.textures.subsurface = LoadTexture(base + "subsurface.png", false);
        m.textures.dirt = LoadTexture(base + "dirt.png", true);
        m.textures.eyeAlbedo = LoadTexture(base + "eye_albedo.png", true);
        m.textures.eyeNormal = LoadTexture(base + "eye_normal.png", false);
    };

    if (key == "boyd") {
        out.characterName = "Boyd";
        out.height = 1.87f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/Boyd.gltf";
        out.animationPaths = {
            "assets/models/characters/Boyd_Idle.gltf",
            "assets/models/characters/Boyd_Run.gltf",
            "assets/models/characters/Boyd_Interact.gltf"
        };

        const std::string base = "assets/textures/characters/Boyd/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);

        out.bodyMaterial.skinTint = glm::vec3(0.94f, 0.78f, 0.66f);
        out.bodyMaterial.subsurfaceStrength = 0.72f;
        out.bodyMaterial.dirtAmount = 0.15f;
        out.hairMaterial.hairColor = glm::vec3(0.08f, 0.06f, 0.05f);
        out.clothingMaterial.clothingTint = glm::vec3(0.95f, 0.95f, 0.95f);
    } else if (key == "jade") {
        out.characterName = "Jade";
        out.height = 1.78f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/Jade.gltf";
        out.animationPaths = {
            "assets/models/characters/Jade_Idle.gltf",
            "assets/models/characters/Jade_Run.gltf",
            "assets/models/characters/Jade_Ability.gltf"
        };

        const std::string base = "assets/textures/characters/Jade/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);

        out.bodyMaterial.skinTint = glm::vec3(0.98f, 0.9f, 0.82f);
        out.bodyMaterial.subsurfaceStrength = 0.65f;
        out.hairMaterial.hairColor = glm::vec3(0.1f, 0.06f, 0.04f);
        out.clothingMaterial.clothingTint = glm::vec3(0.92f, 0.92f, 0.92f);
    } else if (key == "tabitha") {
        out.characterName = "Tabitha";
        out.height = 1.69f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/Tabitha.gltf";
        out.animationPaths = {
            "assets/models/characters/Tabitha_Idle.gltf",
            "assets/models/characters/Tabitha_Run.gltf",
            "assets/models/characters/Tabitha_Listen.gltf"
        };

        const std::string base = "assets/textures/characters/Tabitha/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);

        out.bodyMaterial.skinTint = glm::vec3(1.0f, 0.92f, 0.88f);
        out.bodyMaterial.subsurfaceStrength = 0.75f;
        out.hairMaterial.hairColor = glm::vec3(0.53f, 0.25f, 0.13f);
        out.clothingMaterial.clothingTint = glm::vec3(0.95f, 0.95f, 0.95f);
    } else if (key == "victor") {
        out.characterName = "Victor";
        out.height = 1.75f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/Victor.gltf";
        out.animationPaths = {
            "assets/models/characters/Victor_Idle.gltf",
            "assets/models/characters/Victor_Run.gltf",
            "assets/models/characters/Victor_Memory.gltf"
        };

        const std::string base = "assets/textures/characters/Victor/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);

        out.bodyMaterial.skinTint = glm::vec3(0.93f, 0.9f, 0.85f);
        out.bodyMaterial.subsurfaceStrength = 0.68f;
        out.hairMaterial.hairColor = glm::vec3(0.58f, 0.53f, 0.47f);
        out.clothingMaterial.clothingTint = glm::vec3(0.92f, 0.92f, 0.9f);
    } else if (key == "sara") {
        out.characterName = "Sara";
        out.height = 1.66f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/Sara.gltf";
        out.animationPaths = {
            "assets/models/characters/Sara_Idle.gltf",
            "assets/models/characters/Sara_Run.gltf",
            "assets/models/characters/Sara_Ghost.gltf"
        };

        const std::string base = "assets/textures/characters/Sara/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);

        out.bodyMaterial.skinTint = glm::vec3(0.96f, 0.91f, 0.88f);
        out.bodyMaterial.subsurfaceStrength = 0.74f;
        out.hairMaterial.hairColor = glm::vec3(0.09f, 0.06f, 0.05f);
        out.clothingMaterial.clothingTint = glm::vec3(0.9f, 0.9f, 0.9f);
    } else {
        out.characterName = characterName;
        out.height = 1.78f;
        out.modelScale = glm::vec3(1.0f);
        out.modelPath = "assets/models/characters/" + characterName + ".gltf";
        const std::string base = "assets/textures/characters/" + characterName + "/";
        loadAll(base, out.bodyMaterial);
        loadAll(base, out.clothingMaterial);
        loadAll(base, out.hairMaterial);
    }
}

void CharacterMaterialSystem::BindMaterial(const CharacterMaterial& mat, Shader& shader, int textureUnitStart) {
    const GLuint fallbackAlbedo = EnsureFallbackWhite();
    const GLuint fallbackNormalTex = EnsureFallbackNormal();
    const GLuint fallbackData = EnsureFallbackBlack();

    const GLuint tex[] = {
        mat.textures.albedo ? mat.textures.albedo : fallbackAlbedo,
        mat.textures.normal ? mat.textures.normal : fallbackNormalTex,
        mat.textures.roughness ? mat.textures.roughness : fallbackData,
        mat.textures.metallic ? mat.textures.metallic : fallbackData,
        mat.textures.ao ? mat.textures.ao : fallbackAlbedo,
        mat.textures.emissive ? mat.textures.emissive : fallbackData,
        mat.textures.subsurface ? mat.textures.subsurface : fallbackData,
        mat.textures.dirt ? mat.textures.dirt : fallbackData,
        mat.textures.eyeAlbedo ? mat.textures.eyeAlbedo : fallbackAlbedo,
        mat.textures.eyeNormal ? mat.textures.eyeNormal : fallbackNormalTex,
    };

    const char* samplerNames[] = {
        "albedoMap",
        "normalMap",
        "roughnessMap",
        "metallicMap",
        "aoMap",
        "emissiveMap",
        "subsurfaceMap",
        "dirtMap",
        "eyeAlbedoMap",
        "eyeNormalMap",
    };

    shader.Bind();
    for (int i = 0; i < 10; ++i) {
        glActiveTexture(GL_TEXTURE0 + textureUnitStart + i);
        glBindTexture(GL_TEXTURE_2D, tex[i]);
        shader.SetInt(samplerNames[i], textureUnitStart + i);
    }

    shader.SetVec3("skinTint", mat.skinTint);
    shader.SetVec3("hairColor", mat.hairColor);
    shader.SetVec3("clothingTint", mat.clothingTint);
    shader.SetVec3("emissiveColor", mat.emissiveColor);
    shader.SetFloat("emissiveStrength", mat.emissiveStrength);
    shader.SetFloat("subsurfaceStrength", mat.subsurfaceStrength);
    shader.SetFloat("dirtAmount", mat.dirtAmount);
    shader.SetFloat("wetness", mat.wetness);
    shader.SetBool("useSubsurface", mat.useSubsurface);
    shader.SetBool("useEyeShader", mat.useEyeShader);
}

GLuint CharacterMaterialSystem::LoadTexture(const std::string& path, bool sRGB) {
    auto cached = textureCache.find(path + (sRGB ? "#srgb" : "#lin"));
    if (cached != textureCache.end()) {
        return cached->second;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    GLuint tex = 0;
    if (!data || width <= 0 || height <= 0) {
        std::printf("CharacterMaterialSystem: missing texture %s, using fallback\n", path.c_str());
        const std::string lower = ToLower(path);
        if (lower.find("normal") != std::string::npos) {
            tex = EnsureFallbackNormal();
        } else if (lower.find("ao") != std::string::npos) {
            tex = EnsureFallbackWhite();
        } else {
            tex = EnsureFallbackBlack();
        }
        if (data) {
            stbi_image_free(data);
        }
        textureCache[path + (sRGB ? "#srgb" : "#lin")] = tex;
        return tex;
    }

    GLenum srcFormat = GL_RGB;
    GLenum internalFormat = GL_RGB8;
    if (channels == 1) {
        srcFormat = GL_RED;
        internalFormat = GL_R8;
    } else if (channels == 3) {
        srcFormat = GL_RGB;
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
    } else if (channels == 4) {
        srcFormat = GL_RGBA;
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    } else {
        srcFormat = GL_RGB;
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, srcFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);

    textureCache[path + (sRGB ? "#srgb" : "#lin")] = tex;
    ownedTextures.push_back(tex);
    return tex;
}

GLuint CharacterMaterialSystem::GenerateFallbackTexture(glm::vec4 color) {
    const unsigned char rgba[4] = {
        static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f),
    };

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ownedTextures.push_back(tex);
    return tex;
}

GLuint CharacterMaterialSystem::EnsureFallbackWhite() {
    if (fallbackWhite == 0) {
        fallbackWhite = GenerateFallbackTexture(glm::vec4(1.0f));
    }
    return fallbackWhite;
}

GLuint CharacterMaterialSystem::EnsureFallbackNormal() {
    if (fallbackNormal == 0) {
        fallbackNormal = GenerateFallbackTexture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
    }
    return fallbackNormal;
}

GLuint CharacterMaterialSystem::EnsureFallbackBlack() {
    if (fallbackBlack == 0) {
        fallbackBlack = GenerateFallbackTexture(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    return fallbackBlack;
}
