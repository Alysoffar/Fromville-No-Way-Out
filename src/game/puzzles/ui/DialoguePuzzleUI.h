#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class TextRenderer;

/**
 * DialoguePuzzleUI: Display dialogue options and handle player choices.
 */
class DialoguePuzzleUI {
public:
    void SetDialogue(const std::string& speaker, const std::string& text);
    void SetOptions(const std::vector<std::string>& opts);
    void Update(float dt);
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const;
    int GetSelectedOption() const { return selectedOption; }
    void SelectOption(int idx) { selectedOption = idx; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    std::string speaker;
    std::string text;
    std::vector<std::string> options;
    int selectedOption = -1;
};
