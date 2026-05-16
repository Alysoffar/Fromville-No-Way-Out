#include "game/symbols/SymbolSystem.h"

#include <algorithm>
#include <cmath>

namespace {
static SymbolSystem* g_symbolSystem = nullptr;
}

SymbolSystem& SymbolSystem::Instance() {
    if (!g_symbolSystem) {
        g_symbolSystem = new SymbolSystem();
    }
    return *g_symbolSystem;
}

SymbolSystem::SymbolSystem() : decodingProgress(0.0f) {
    // Predefine town language patterns
    patterns["AWAKENING"] = {
        "The Awakening Sequence",
        {SymbolType::Knowledge, SymbolType::Power, SymbolType::Truth, SymbolType::Awakening},
        "The ritual cycle begins every 25 years when the markers align."
    };

    patterns["CROSSING"] = {
        "The Crossing",
        {SymbolType::Crossing, SymbolType::Void, SymbolType::Sacrifice},
        "Through crossing and void, sacrifice opens the path."
    };

    patterns["WITNESS"] = {
        "The Children Watch",
        {SymbolType::Witness, SymbolType::Knowledge, SymbolType::Power},
        "The children are the eyes and guides of the town."
    };

    patterns["VOID_TRUTH"] = {
        "Truth in the Void",
        {SymbolType::Truth, SymbolType::Void, SymbolType::Awakening},
        "Where there is nothing, truth emerges."
    };
}

void SymbolSystem::Update(float dt) {
    if (symbolSightActive) {
        symbolSightTimer -= dt;
        if (symbolSightTimer <= 0.0f) {
            symbolSightActive = false;
        }
    }

    if (rearrangementIntensity > 0.0f) {
        rearrangementIntensity = std::max(0.0f, rearrangementIntensity - dt * 0.5f);
    }

    if (wallBleedIntensity > 0.0f) {
        wallBleedIntensity = std::max(0.0f, wallBleedIntensity - dt * 0.8f);
    }

    // Pulse symbols at matching frequencies
    for (auto& symbol : allSymbols) {
        if (symbol.isPulsing) {
            symbol.scale = 1.0f + 0.15f * std::sin(symbolSightTimer * symbol.frequency * 4.0f);
        }
    }
}

void SymbolSystem::Render() const {
    // Rendered by overlay/world system when Symbol Sight is active
}

void SymbolSystem::RegisterSymbol(const Symbol& symbol) {
    allSymbols.push_back(symbol);
    if (symbol.isHidden) {
        hiddenSymbols.push_back(symbol);
    } else {
        visibleSymbols.push_back(symbol);
    }
}

void SymbolSystem::RegisterPattern(const SymbolPattern& pattern) {
    patterns[pattern.name] = pattern;
}

void SymbolSystem::RevealSymbols(float duration) {
    symbolSightActive = true;
    symbolSightDuration = duration;
    symbolSightTimer = duration;

    // Move hidden symbols to visible
    for (auto& symbol : hiddenSymbols) {
        symbol.isHidden = false;
        visibleSymbols.push_back(symbol);
    }
    hiddenSymbols.clear();

    // Enable pulsing
    for (auto& symbol : visibleSymbols) {
        symbol.isPulsing = true;
    }
}

void SymbolSystem::UpdateSymbolSight(float dt) {
    if (!symbolSightActive) {
        for (auto& symbol : visibleSymbols) {
            symbol.isPulsing = false;
            symbol.scale = 1.0f;
        }
    }
}

bool SymbolSystem::CheckPatternMatch(const std::vector<SymbolType>& playerSequence) {
    for (const auto& [name, pattern] : patterns) {
        if (pattern.sequence == playerSequence) {
            return true;
        }
    }
    return false;
}

std::string SymbolSystem::DecodePattern(const std::vector<SymbolType>& sequence) {
    for (const auto& [name, pattern] : patterns) {
        if (pattern.sequence == sequence) {
            AdvanceDecoding(0.2f);
            return pattern.meaning;
        }
    }
    return "...the symbols remain obscure.";
}

std::string SymbolSystem::GetTownLanguageHint(int fragmentIndex) const {
    if (fragmentIndex >= 0 && fragmentIndex < static_cast<int>(kLanguageFragments.size())) {
        return kLanguageFragments[fragmentIndex];
    }
    return "";
}

void SymbolSystem::TriggerSymbolRearrangement() {
    rearrangementIntensity = 1.0f;
    for (auto& symbol : visibleSymbols) {
        symbol.rotation += 45.0f;
        if (symbol.rotation >= 360.0f) {
            symbol.rotation -= 360.0f;
        }
    }
}

void SymbolSystem::BleedSymbolsOnWall() {
    wallBleedIntensity = 1.0f;
    for (auto& symbol : visibleSymbols) {
        symbol.scale *= 1.2f;
    }
}

void SymbolSystem::PulseSymbolsAtFrequency(int frequency) {
    for (auto& symbol : visibleSymbols) {
        if (symbol.frequency == frequency) {
            symbol.isPulsing = true;
        }
    }
}

const std::vector<Symbol>& SymbolSystem::GetAllSymbols() const {
    return allSymbols;
}
