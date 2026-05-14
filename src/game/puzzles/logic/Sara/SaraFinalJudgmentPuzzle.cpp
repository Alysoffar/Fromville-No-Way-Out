#include "game/puzzles/logic/Sara/SaraFinalJudgmentPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

SaraFinalJudgmentPuzzle::SaraFinalJudgmentPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void SaraFinalJudgmentPuzzle::Start() {
    Reset();
    statusLine = "Sara prepares the final confrontation. Every contradiction matters.";
    statusTimer = 2.8f;
    PlaySound("judgment_begin");
}

void SaraFinalJudgmentPuzzle::AdvancePhase() {
    if (phase == Phase::Investigation && evidenceExposed >= 3) {
        phase = Phase::CrossExamination;
        statusLine = "The evidence is enough. Now force the torturer to answer.";
        statusTimer = 2.4f;
        PlaySound("puzzle_tick");
    } else if (phase == Phase::CrossExamination && truth.GetTruth() >= 90.0f) {
        phase = Phase::Collapse;
        statusLine = "The defenses collapse. The final choice is all that remains.";
        statusTimer = 2.4f;
        PlaySound("puzzle_tick");
    }
}

void SaraFinalJudgmentPuzzle::ExposeEvidence(int index) {
    if (index == evidenceIndex) {
        evidenceExposed++;
        truth.AddTruth(28.0f);
        pressure = std::max(0.0f, pressure - 8.0f);
        statusLine = "That contradiction lands. The story starts to split apart.";
        statusTimer = 2.0f;
        overlay.Trigger(0.35f);
        atmosphere.Pulse(9.0f);
        PlaySound("puzzle_tick");
        evidenceIndex = (evidenceIndex + 1) % static_cast<int>(evidence.size());
        if (evidenceExposed >= 3) {
            AdvancePhase();
        }
    } else {
        wrongPressures++;
        pressure += 14.0f;
        sanity.AddPressure(12.0f);
        tracker.Record({"Sara", evidence[static_cast<std::size_t>(index)], static_cast<int>(timer * 1000.0f)});
        statusLine = "Wrong evidence. The room turns the mistake back on you.";
        statusTimer = 2.0f;
        overlay.Trigger(0.18f);
        atmosphere.Pulse(6.0f);
        PlaySound("puzzle_fail");
    }
}

void SaraFinalJudgmentPuzzle::CommitFinalChoice(MoralChoice choice) {
    finalChoice = choice;
    choiceMade = true;
    phase = Phase::Verdict;
    MoralCorruptionSystem::Instance().RecordChoice(choice);

    switch (choice) {
        case MoralChoice::Mercy:
            solvedMessage = "Sara chooses mercy and breaks the cycle.";
            break;
        case MoralChoice::Exposure:
            solvedMessage = "Sara exposes the truth and condemns the lie publicly.";
            break;
        case MoralChoice::Revenge:
            solvedMessage = "Sara lets vengeance burn the room clean.";
            break;
        case MoralChoice::Sacrifice:
            solvedMessage = "Sara takes the burden onto herself and seals the corruption.";
            break;
    }

    solved = true;
    statusLine = "The final judgment is spoken.";
    statusTimer = 3.5f;
    PlaySound("puzzle_complete");
}

void SaraFinalJudgmentPuzzle::Update(float dt, const InputContext& input) {
    timer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    pressure = std::max(0.0f, pressure - dt * 1.5f);

    overlay.Update(dt);
    atmosphere.Escalate(dt);
    sanity.Update(dt);

    if (sanity.IsBroken()) {
        solved = false;
        statusLine = "The confrontation overwhelms Sara.";
        PlaySound("puzzle_fail");
        return;
    }

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (phase == Phase::Investigation) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) evidenceIndex = 0;
        if (input.IsActionPressed(InputAction::PuzzleOption2)) evidenceIndex = 1;
        if (input.IsActionPressed(InputAction::PuzzleOption3)) evidenceIndex = 2;
        if (input.IsActionPressed(InputAction::PuzzleOption4)) evidenceIndex = 3;
        if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
            ExposeEvidence(evidenceIndex);
        }
    } else if (phase == Phase::CrossExamination) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) evidenceIndex = 0;
        if (input.IsActionPressed(InputAction::PuzzleOption2)) evidenceIndex = 1;
        if (input.IsActionPressed(InputAction::PuzzleOption3)) evidenceIndex = 2;
        if (input.IsActionPressed(InputAction::PuzzleOption4)) evidenceIndex = 3;
        if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
            ExposeEvidence(evidenceIndex);
        }
    } else if (phase == Phase::Collapse) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) CommitFinalChoice(MoralChoice::Mercy);
        if (input.IsActionPressed(InputAction::PuzzleOption2)) CommitFinalChoice(MoralChoice::Exposure);
        if (input.IsActionPressed(InputAction::PuzzleOption3)) CommitFinalChoice(MoralChoice::Revenge);
        if (input.IsActionPressed(InputAction::PuzzleOption4)) CommitFinalChoice(MoralChoice::Sacrifice);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        pressure += 5.0f;
        sanity.AddPressure(4.0f);
        statusLine = "The room hears your hesitation.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }
}

void SaraFinalJudgmentPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(1.0f, 0.72f, 0.72f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.48f);
    const glm::vec3 calmColor(0.8f, 0.95f, 1.0f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Break the witness down with evidence, not force.", 72.0f, topY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    const char* phaseText = "PHASE: UNKNOWN";
    switch (phase) {
        case Phase::Investigation: phaseText = "PHASE: INVESTIGATION"; break;
        case Phase::CrossExamination: phaseText = "PHASE: CROSS-EXAMINATION"; break;
        case Phase::Collapse: phaseText = "PHASE: COLLAPSE"; break;
        case Phase::Verdict: phaseText = "PHASE: VERDICT"; break;
    }
    textRenderer.RenderText(phaseText, 72.0f, topY + 128.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (evidenceIndex == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + evidence[static_cast<std::size_t>(i)],
                                72.0f, topY + 172.0f + static_cast<float>(i) * 28.0f, 0.30f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Evidence exposed: " + std::to_string(evidenceExposed) + "/3", 72.0f, static_cast<float>(screenHeight) - 164.0f, 0.36f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Pressure: " + std::string(static_cast<int>(pressure * 0.6f), '!'), 72.0f, static_cast<float>(screenHeight) - 132.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 96.0f, 0.34f, (solved ? calmColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (phase == Phase::Collapse) {
        textRenderer.RenderText("[1-4] choose final judgment", 72.0f, static_cast<float>(screenHeight) - 62.0f, 0.34f, titleColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE CONFRONTATION BREAKS THE TORTURER", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] select evidence or verdict  [ENTER] act  [ESC] steady  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, titleColor * alpha, screenWidth, screenHeight);
}

std::string SaraFinalJudgmentPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << static_cast<int>(phase) << ';' << evidenceIndex << ';' << evidenceExposed << ';'
           << wrongPressures << ';' << (choiceMade ? 1 : 0) << ';' << static_cast<int>(finalChoice) << ';' << timer << ';' << pressure;
    return stream.str();
}

void SaraFinalJudgmentPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); phase = static_cast<Phase>(token.empty() ? 0 : std::stoi(token));
    std::getline(stream, token, ';'); evidenceIndex = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); evidenceExposed = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); wrongPressures = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); choiceMade = (token == "1");
    std::getline(stream, token, ';'); finalChoice = static_cast<MoralChoice>(token.empty() ? 1 : std::stoi(token));
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); pressure = token.empty() ? 0.0f : std::stof(token);

    evidenceIndex = std::clamp(evidenceIndex, 0, 3);
}

void SaraFinalJudgmentPuzzle::Reset() {
    evidenceIndex = 0;
    evidenceExposed = 0;
    wrongPressures = 0;
    phase = Phase::Investigation;
    finalChoice = MoralChoice::Exposure;
    choiceMade = false;
    solved = false;
    timer = 0.0f;
    pressure = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
    sanity = SanitySystem();
    truth = TruthSystem();
    tracker = ContradictionTracker();
    overlay = HallucinationOverlay();
    atmosphere = AtmosphereSystem();
}
