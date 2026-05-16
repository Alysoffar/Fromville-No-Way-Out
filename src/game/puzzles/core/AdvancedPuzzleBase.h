#pragma once

#include <string>
#include <nlohmann/json.hpp>

/**
 * AdvancedPuzzleBase: Base class for psychologically-driven puzzles
 * Extends traditional puzzle systems with sanity, truth, contradictions, and atmosphere.
 */
class AdvancedPuzzleBase {
public:
    virtual ~AdvancedPuzzleBase() = default;

    // Lifecycle
    virtual void Initialize() = 0;
    virtual void Activate() = 0;
    virtual void SuspendGameplay() = 0;
    virtual void Update(float dt) = 0;
    virtual void HandleInput(int action, int value) = 0;
    virtual void Render() = 0;
    virtual void Complete() = 0;
    virtual void Fail() = 0;
    virtual void Cleanup() = 0;
    virtual void RestoreGameplay() = 0;

    // Persistence
    virtual nlohmann::json SerializeState() const = 0;
    virtual void DeserializeState(const nlohmann::json& j) = 0;

    // Utility
    virtual std::string GetName() const = 0;
};
