#include "game/puzzles/logic/Boyed/EvidenceBoardPuzzle.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "engine/renderer/TextRenderer.h"

EvidenceBoardPuzzle::EvidenceBoardPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

float EvidenceBoardPuzzle::TriangleArea(const glm::ivec2& a, const glm::ivec2& b, const glm::ivec2& c) {
    const float signedArea = 0.5f * static_cast<float>(a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    return std::abs(signedArea);
}

bool EvidenceBoardPuzzle::IsSolvedByGeometry() const {
    return std::abs(TriangleArea(itemPoints[0], itemPoints[1], itemPoints[2]) - cultTerritoryArea) <= tolerance;
}

void EvidenceBoardPuzzle::ClampItem(glm::ivec2& item) const {
    item.x = std::max(-4, std::min(4, item.x));
    item.y = std::max(-4, std::min(4, item.y));
}

void EvidenceBoardPuzzle::Start() {
    PlaySound("puzzle_open");
}

void EvidenceBoardPuzzle::Update(float, const InputContext& input) {
    if (solved) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
            if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            selectedItem = option;
            PlaySound("puzzle_tick");
        }
    }

    glm::ivec2& item = itemPoints[selectedItem];
        if (input.IsActionPressed(InputAction::PuzzleUp)) {
        ++item.y;
        PlaySound("puzzle_tick");
    }
        if (input.IsActionPressed(InputAction::PuzzleDown)) {
        --item.y;
        PlaySound("puzzle_tick");
    }
        if (input.IsActionPressed(InputAction::PuzzleLeft)) {
        --item.x;
        PlaySound("puzzle_tick");
    }
        if (input.IsActionPressed(InputAction::PuzzleRight)) {
        ++item.x;
        PlaySound("puzzle_tick");
    }

    for (auto& point : itemPoints) {
        ClampItem(point);
    }

    if (IsSolvedByGeometry()) {
        solved = true;
        PlaySound("puzzle_solve");
    }
}

void EvidenceBoardPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);

    // Left Panel status (matching Jade's design)
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ MAPPING STATUS", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);
    
    for (int i = 0; i < 3; ++i) {
        std::ostringstream line;
        line << "Item " << static_cast<char>('A' + i)
             << " Coord: (" << itemPoints[i].x << "," << itemPoints[i].y << ")";
        const glm::vec3 color = (i == selectedItem) ? okColor : glm::vec3(0.86f, 0.86f, 0.78f);
        textRenderer.RenderText(line.str(), leftPanelX, leftPanelY - 40.0f - (static_cast<float>(i) * 24.0f), 0.52f, color * alpha, screenWidth, screenHeight);
    }

    std::ostringstream areaLine;
    areaLine << "Area: " << TriangleArea(itemPoints[0], itemPoints[1], itemPoints[2]) << " / Target " << cultTerritoryArea;
    textRenderer.RenderText(areaLine.str(), leftPanelX, leftPanelY - 120.0f, 0.50f, accentColor * alpha, screenWidth, screenHeight);

    if (solved) {
        textRenderer.RenderText("Solved:", leftPanelX, leftPanelY - 150.0f, 0.50f, okColor * alpha, screenWidth, screenHeight);
        textRenderer.RenderText("Map verified.", leftPanelX, leftPanelY - 174.0f, 0.46f, okColor * alpha, screenWidth, screenHeight);
    }

    // Centered interactive board
    const float gridSize = 280.0f;
    const float gridCenterX = centerX;
    const float gridCenterY = baseY + 80.0f;
    const int half = 4;
    const float cellSize = gridSize / (half * 2);
    const float startX = gridCenterX - gridSize * 0.5f + cellSize * 0.5f;
    const float startY = gridCenterY + gridSize * 0.5f - cellSize * 0.5f;

    // draw marker squares for items A/B/C
    for (int k = 0; k < 3; ++k) {
        const int gx = itemPoints[k].x;
        const int gy = itemPoints[k].y;
        const int cx = std::max(-half, std::min(half, gx));
        const int cy = std::max(-half, std::min(half, gy));

        const float px = startX + (static_cast<float>(cx + half) * cellSize);
        const float py = startY - (static_cast<float>(cy + half) * cellSize);

        const glm::vec3 bg = (k == selectedItem) ? glm::vec3(0.18f, 0.12f, 0.08f) : glm::vec3(0.12f, 0.14f, 0.16f);
        const glm::vec3 fg = (k == selectedItem) ? glm::vec3(1.0f, 0.85f, 0.7f) : glm::vec3(0.9f, 0.9f, 0.86f);

        textRenderer.RenderText("■", px - 18.0f, py + 18.0f, 2.4f, bg * alpha, screenWidth, screenHeight);
        std::ostringstream label;
        label << static_cast<char>('A' + k);
        textRenderer.RenderText(label.str(), px - 10.0f, py - 6.0f, 1.6f, fg * alpha, screenWidth, screenHeight);
    }

    // Draw coordinate labels and light grid dots
    for (int gy = half; gy >= -half; --gy) {
        for (int gx = -half; gx <= half; ++gx) {
            const float px = startX + (static_cast<float>(gx + half) * cellSize);
            const float py = startY - (static_cast<float>(gy + half) * cellSize);
            textRenderer.RenderText("·", px - 4.0f, py - 6.0f, 0.48f, glm::vec3(0.78f,0.78f,0.78f) * alpha * 0.6f, screenWidth, screenHeight);
        }
    }

    if (solved) {
        textRenderer.RenderText("★ EVIDENCE ALIGNED ★", centerX - 160.0f, baseY + 260.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
}

std::string EvidenceBoardPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedItem << ';';
    for (std::size_t i = 0; i < itemPoints.size(); ++i) {
        if (i > 0) {
            stream << ',';
        }
        stream << itemPoints[i].x << ':' << itemPoints[i].y;
    }
    return stream.str();
}

void EvidenceBoardPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';');
    solved = token == "1";
    std::getline(stream, token, ';');
    selectedItem = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';');
    std::stringstream points(token);
    for (std::size_t i = 0; i < itemPoints.size(); ++i) {
        std::getline(points, token, ',');
        std::stringstream xy(token);
        std::getline(xy, token, ':');
        itemPoints[i].x = token.empty() ? 0 : std::stoi(token);
        std::getline(xy, token, ':');
        itemPoints[i].y = token.empty() ? 0 : std::stoi(token);
    }
}

void EvidenceBoardPuzzle::Reset() {
    itemPoints = { glm::ivec2(0, 0), glm::ivec2(2, 1), glm::ivec2(-1, 3) };
    selectedItem = 0;
    solved = false;
}