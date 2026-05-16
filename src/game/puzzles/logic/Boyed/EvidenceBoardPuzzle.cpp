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
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);
    const float baseY = static_cast<float>(screenHeight) * 0.56f;

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("MAP THE EVIDENCE TRIANGLE", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("1-3 SELECT  ARROWS MOVE POINTS", 74.0f, static_cast<float>(screenHeight) - 200.0f, 0.35f, textColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 3; ++i) {
        std::ostringstream line;
        line << (i + 1) << ". ITEM " << static_cast<char>('A' + i)
             << "  COORD(" << itemPoints[i].x << "," << itemPoints[i].y << ")"
             << (i == selectedItem ? "  <" : "");
        const glm::vec3 color = (i == selectedItem) ? glm::vec3(1.0f, 0.6f, 0.45f) : glm::vec3(0.86f, 0.86f, 0.78f);
        textRenderer.RenderText(line.str(), 74.0f, baseY - (static_cast<float>(i) * 28.0f), 0.42f, color * alpha, screenWidth, screenHeight);
    }

    std::ostringstream areaLine;
    areaLine << "TRIANGLE AREA: " << TriangleArea(itemPoints[0], itemPoints[1], itemPoints[2]) << " / TARGET " << cultTerritoryArea;
    textRenderer.RenderText(areaLine.str(), 74.0f, baseY - 118.0f, 0.40f, accentColor * alpha, screenWidth, screenHeight);

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, baseY - 160.0f, 0.45f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }

    // Render a clear 9x9 coordinate grid with visible A/B/C markers (-4..4)
    const float gridSize = 280.0f;
    const float centerX = static_cast<float>(screenWidth) * 0.66f;
    const float centerY = baseY - 60.0f;
    const int half = 4;
    const float cellSize = gridSize / (half * 2);
    const float startX = centerX - gridSize * 0.5f + cellSize * 0.5f;
    const float startY = centerY + gridSize * 0.5f - cellSize * 0.5f;

    // draw marker squares for items A/B/C
    for (int k = 0; k < 3; ++k) {
        const int gx = itemPoints[k].x;
        const int gy = itemPoints[k].y;
        // clamp to grid
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
            // faint dot for empty cells
            textRenderer.RenderText("·", px - 4.0f, py - 6.0f, 0.48f, glm::vec3(0.78f,0.78f,0.78f) * alpha * 0.6f, screenWidth, screenHeight);
        }
    }

    // Explicit control hint
    textRenderer.RenderText("Press 1-3 to select an item  |  Arrow keys move selected item", centerX - 340.0f, centerY + gridSize * 0.6f, 0.46f, glm::vec3(0.95f,0.95f,0.9f) * alpha, screenWidth, screenHeight);
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