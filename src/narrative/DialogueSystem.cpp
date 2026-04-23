#include "DialogueSystem.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "NarrativeEngine.h"
#include "characters/Character.h"
#include "characters/Sara.h"
#include "core/EventBus.h"
#include "core/InputManager.h"
#include "renderer/Renderer.h"
#include "world/GlyphPuzzle.h"

namespace {

/// Characters per second for the typewriter reveal effect.
constexpr float kCharsPerSecond = 40.0f;

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DialogueSystem::DialogueSystem(GlyphRegistry* glyphs, NarrativeEngine* narrative)
    : glyphs_(glyphs), narrative_(narrative) {}

// ---------------------------------------------------------------------------
// Data loading
// ---------------------------------------------------------------------------

void DialogueSystem::LoadDialogues(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::printf("DialogueSystem: cannot open '%s'\n", jsonPath.c_str());
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::printf("DialogueSystem: JSON parse error in '%s': %s\n",
                    jsonPath.c_str(), e.what());
        return;
    }

    for (auto& [npcName, lineMap] : j.items()) {
        for (auto& [lineId, lineData] : lineMap.items()) {
            DialogueLine line;
            line.speaker      = lineData.value("speaker", npcName);
            line.text         = lineData.value("text", "");
            line.requiresFlag = lineData.value("requiresFlag", "");
            line.setsFlag     = lineData.value("setsFlag", "");
            line.endsDialogue = lineData.value("endsDialogue", false);

            if (lineData.contains("responses") && lineData["responses"].is_array()) {
                for (const auto& rj : lineData["responses"]) {
                    DialogueResponse resp;
                    resp.text                = rj.value("text", "");
                    resp.requiresFlag        = rj.value("requiresFlag", "");
                    resp.nextLineId          = rj.value("nextLineId", "");
                    resp.setsFlag            = rj.value("setsFlag", "");
                    resp.addsSaraRedemption  = rj.value("addsSaraRedemption", 0.0f);
                    line.responses.push_back(std::move(resp));
                }
            }

            dialogues_[npcName][lineId] = std::move(line);
        }
    }
    std::printf("DialogueSystem: loaded dialogues from '%s'\n", jsonPath.c_str());
}

// ---------------------------------------------------------------------------
// Conversation control
// ---------------------------------------------------------------------------

void DialogueSystem::StartConversation(const std::string& npcName, Character* player) {
    auto it = dialogues_.find(npcName);
    if (it == dialogues_.end()) {
        std::printf("DialogueSystem: no dialogue data for NPC '%s'\n", npcName.c_str());
        return;
    }

    // Find the first line that has no requiresFlag or whose flag is satisfied.
    // By convention, the first entry is the root; we look for id == "root" or
    // the first element whose requiresFlag is satisfied.
    const std::string* startId = nullptr;
    for (const auto& [id, line] : it->second) {
        if (FlagSatisfied(line.requiresFlag)) {
            startId = &id;
            break;
        }
    }
    // Prefer the conventional "root" node if it exists.
    if (it->second.count("root") && FlagSatisfied(it->second.at("root").requiresFlag)) {
        static const std::string rootKey = "root";
        startId = &rootKey;
    }

    if (!startId) {
        std::printf("DialogueSystem: no valid start line for '%s'\n", npcName.c_str());
        return;
    }

    active_          = true;
    currentNPC_      = npcName;
    currentLineId_   = *startId;
    currentPlayer_   = player;
    textRevealTimer_ = 0.0f;
    revealedChars_   = 0;
    selectedResponse_= 0;

    // Mark the line's setsFlag
    DialogueLine* line = GetLine(currentLineId_);
    if (line && !line->setsFlag.empty()) {
        narrative_->SetFlag(line->setsFlag);
    }

    EventBus::Get().Fire(GameEvent::DIALOGUE_STARTED, {{"npc", currentNPC_}});
}

void DialogueSystem::EndConversation() {
    if (!active_) {
        return;
    }
    active_ = false;
    EventBus::Get().Fire(GameEvent::DIALOGUE_ENDED, {{"npc", currentNPC_}});
    currentNPC_.clear();
    currentLineId_.clear();
    currentPlayer_ = nullptr;
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void DialogueSystem::Update(float dt, InputManager& input) {
    if (!active_) {
        return;
    }

    DialogueLine* line = GetLine(currentLineId_);
    if (!line) {
        EndConversation();
        return;
    }

    // Advance typewriter
    textRevealTimer_ += dt;
    const int totalChars = static_cast<int>(line->text.size());
    revealedChars_ = std::min(
        totalChars,
        static_cast<int>(textRevealTimer_ * kCharsPerSecond));

    const bool fullyRevealed = (revealedChars_ >= totalChars);

    // --- Input ---
    // Skip typewriter
    if (!fullyRevealed && input.IsKeyPressed(GLFW_KEY_SPACE)) {
        revealedChars_   = totalChars;
        textRevealTimer_ = static_cast<float>(totalChars) / kCharsPerSecond;
        return;
    }

    if (!fullyRevealed) {
        return;   // wait until text is done before allowing navigation
    }

    // Gather visible responses
    std::vector<int> visibleIndices;
    for (int i = 0; i < static_cast<int>(line->responses.size()); ++i) {
        if (FlagSatisfied(line->responses[i].requiresFlag)) {
            visibleIndices.push_back(i);
        }
    }

    if (!visibleIndices.empty()) {
        // Navigate responses
        if (input.IsKeyPressed(GLFW_KEY_UP)) {
            selectedResponse_ = std::max(0, selectedResponse_ - 1);
        }
        if (input.IsKeyPressed(GLFW_KEY_DOWN)) {
            selectedResponse_ = std::min(
                static_cast<int>(visibleIndices.size()) - 1,
                selectedResponse_ + 1);
        }
        if (input.IsKeyPressed(GLFW_KEY_ENTER) ||
            input.IsKeyPressed(GLFW_KEY_SPACE)) {
            SelectResponse(visibleIndices[selectedResponse_]);
        }
    } else {
        // No responses — any key / space advances or ends dialogue
        if (input.IsKeyPressed(GLFW_KEY_ENTER) ||
            input.IsKeyPressed(GLFW_KEY_SPACE)) {
            if (line->endsDialogue || currentLineId_.empty()) {
                EndConversation();
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void DialogueSystem::Draw(Renderer& renderer) {
    if (!active_) {
        return;
    }
    DialogueLine* line = GetLine(currentLineId_);
    if (!line) {
        return;
    }
    DrawDialogueBox(renderer, *line);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

DialogueLine* DialogueSystem::GetLine(const std::string& lineId) {
    auto npcIt = dialogues_.find(currentNPC_);
    if (npcIt == dialogues_.end()) {
        return nullptr;
    }
    auto lineIt = npcIt->second.find(lineId);
    if (lineIt == npcIt->second.end()) {
        return nullptr;
    }
    return &lineIt->second;
}

bool DialogueSystem::FlagSatisfied(const std::string& flag) const {
    if (flag.empty()) {
        return true;
    }
    return narrative_ && narrative_->GetFlag(flag);
}

void DialogueSystem::SelectResponse(int index) {
    DialogueLine* line = GetLine(currentLineId_);
    if (!line || index < 0 || index >= static_cast<int>(line->responses.size())) {
        return;
    }

    const DialogueResponse& resp = line->responses[index];

    // Apply side-effects
    if (!resp.setsFlag.empty() && narrative_) {
        narrative_->SetFlag(resp.setsFlag);
    }

    if (resp.addsSaraRedemption != 0.0f && currentPlayer_) {
        if (auto* sara = dynamic_cast<Sara*>(currentPlayer_)) {
            sara->AddRedemptionScore(resp.addsSaraRedemption);
        }
    }

    // Advance or end
    if (resp.nextLineId.empty()) {
        EndConversation();
        return;
    }

    currentLineId_   = resp.nextLineId;
    textRevealTimer_ = 0.0f;
    revealedChars_   = 0;
    selectedResponse_= 0;

    // Mark the new line's setsFlag
    DialogueLine* nextLine = GetLine(currentLineId_);
    if (nextLine) {
        if (!nextLine->setsFlag.empty() && narrative_) {
            narrative_->SetFlag(nextLine->setsFlag);
        }
        if (nextLine->endsDialogue) {
            // Allow one frame for Draw() to show it, then end.
            // We mark it here; the next Update will call End.
        }
    }
}

void DialogueSystem::DrawDialogueBox(Renderer& /*renderer*/, const DialogueLine& line) {
    // Renderer doesn't yet expose a 2-D text/UI API; log to stdout as a
    // stand-in.  Replace with actual UI draw calls (e.g., Freetype quads)
    // once the UI layer is wired up.
    const std::string visibleText = line.text.substr(0, static_cast<std::size_t>(revealedChars_));
    std::printf("[%s]: %s\n", line.speaker.c_str(), visibleText.c_str());

    auto npcIt = dialogues_.find(currentNPC_);
    if (npcIt == dialogues_.end()) {
        return;
    }

    const bool fullyRevealed = (revealedChars_ >= static_cast<int>(line.text.size()));
    if (fullyRevealed) {
        int visibleIdx = 0;
        for (int i = 0; i < static_cast<int>(line.responses.size()); ++i) {
            if (FlagSatisfied(line.responses[i].requiresFlag)) {
                const char* cursor = (visibleIdx == selectedResponse_) ? "> " : "  ";
                std::printf("  %s%d. %s\n", cursor, visibleIdx + 1,
                            line.responses[i].text.c_str());
                ++visibleIdx;
            }
        }
    }
}
