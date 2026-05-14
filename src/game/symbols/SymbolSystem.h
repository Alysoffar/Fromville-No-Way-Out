#pragma once

#include <array>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

enum class SymbolType {
    Knowledge,      // Circular rays
    Power,          // Interlocking triangles
    Truth,          // Eye in pyramid
    Crossing,       // Two circles meeting
    Sacrifice,      // Bone symbol
    Awakening,      // Spiral
    Void,           // Anti-symbol (empty space)
    Witness         // Watching eyes
};

struct Symbol {
    SymbolType type;
    glm::vec3 position;
    float rotation = 0.0f;
    float scale = 1.0f;
    int frequency = 1;  // How often symbol appears/repeats
    bool isHidden = true;
    bool isPulsing = false;
};

struct SymbolPattern {
    std::string name;
    std::vector<SymbolType> sequence;
    std::string meaning;
    int difficulty = 1;
};

class SymbolSystem {
public:
    static SymbolSystem& Instance();

    void Update(float dt);
    void Render() const;

    // Register symbols in the world
    void RegisterSymbol(const Symbol& symbol);
    void RegisterPattern(const SymbolPattern& pattern);

    // Jade's Symbol Sight ability
    void RevealSymbols(float duration);
    void UpdateSymbolSight(float dt);

    // Query symbols
    const std::vector<Symbol>& GetVisibleSymbols() const { return visibleSymbols; }
    const std::vector<Symbol>& GetHiddenSymbols() const { return hiddenSymbols; }
    const std::vector<Symbol>& GetAllSymbols() const;

    // Pattern matching
    bool CheckPatternMatch(const std::vector<SymbolType>& playerSequence);
    std::string DecodePattern(const std::vector<SymbolType>& sequence);

    // Town language
    std::string GetTownLanguageHint(int fragmentIndex) const;
    float GetDecodingProgress() const { return decodingProgress; }
    void AdvanceDecoding(float amount) { decodingProgress = std::clamp(decodingProgress + amount, 0.0f, 1.0f); }

    // Environmental effects
    void TriggerSymbolRearrangement();
    void BleedSymbolsOnWall();
    void PulseSymbolsAtFrequency(int frequency);

private:
    SymbolSystem();
    ~SymbolSystem() = default;

    std::vector<Symbol> visibleSymbols;
    std::vector<Symbol> hiddenSymbols;
    std::unordered_map<std::string, SymbolPattern> patterns;
    std::vector<Symbol> allSymbols;

    float symbolSightTimer = 0.0f;
    float symbolSightDuration = 0.0f;
    bool symbolSightActive = false;

    float decodingProgress = 0.0f;
    float rearrangementIntensity = 0.0f;
    float wallBleedIntensity = 0.0f;

    // Town language fragments (encoded truth)
    const std::array<const char*, 5> kLanguageFragments = {{
        "The crossing opens once every generation.",
        "Power seeks to awaken.",
        "Truth must be guarded or chaos breaks free.",
        "The children are the eyes of the town.",
        "When the markers align, the veil thins."
    }};
};
