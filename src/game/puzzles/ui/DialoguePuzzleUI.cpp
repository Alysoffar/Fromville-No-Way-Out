#include "game/puzzles/ui/DialoguePuzzleUI.h"

void DialoguePuzzleUI::SetDialogue(const std::string& spk, const std::string& txt) {
    speaker = spk;
    text = txt;
}

void DialoguePuzzleUI::SetOptions(const std::vector<std::string>& opts) {
    options = opts;
    selectedOption = -1;
}

void DialoguePuzzleUI::Update(float /*dt*/) {
    // Update logic here (e.g., fade-in, text reveal)
}

void DialoguePuzzleUI::Render(TextRenderer& /*textRenderer*/, int /*screenWidth*/, int /*screenHeight*/, float /*alpha*/) const {
    // Placeholder: Real rendering would display speaker, text, and options
}

nlohmann::json DialoguePuzzleUI::Serialize() const {
    nlohmann::json j;
    j["speaker"] = speaker;
    j["text"] = text;
    j["selectedOption"] = selectedOption;
    j["options"] = options;
    return j;
}

void DialoguePuzzleUI::Deserialize(const nlohmann::json& j) {
    speaker = j.value("speaker", std::string());
    text = j.value("text", std::string());
    selectedOption = j.value("selectedOption", -1);
    options = j.value("options", std::vector<std::string>());
}
