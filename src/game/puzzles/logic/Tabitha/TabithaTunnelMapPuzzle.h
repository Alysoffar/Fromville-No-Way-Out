#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"
#include "game/tunnels/TunnelMappingSystem.h"

class TextRenderer;

class TabithaTunnelMapPuzzle final : public PuzzleBase {
public:
    TabithaTunnelMapPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Flooded Tunnel Pressure Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> valves = {{"North Pump", "East Gate", "South Drain", "West Spillway"}};
    int selectedValve = 0;
    int reroutes = 0;
    bool solved = false;

    float waterLevel = 18.0f;
    float pressureLevel = 20.0f;
    float pulse = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    int ExpectedValve() const;
    void RerouteValve(int index);
};