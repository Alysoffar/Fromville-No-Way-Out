#include "game/puzzles/logic/Sara/SaraBreathingClosetPuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

SaraBreathingClosetPuzzle::SaraBreathingClosetPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {}

void SaraBreathingClosetPuzzle::Start() {
    Reset();
    hideTimer = 28.0f; // survive for 28 seconds
    eventTimer = 2.0f;
    statusLine = "You curl into the dark, listening to the room breathe without you.";
    statusTimer = 3.0f;
    PlaySound("puzzle_open");
}

void SaraBreathingClosetPuzzle::TriggerSoundEvent() {
    const int r = rand() % 5;
    switch (r) {
        case 0:
            statusLine = "Footsteps scrape past the doorframe. A drawer slides open downstairs.";
            panic += 0.06f;
            PlaySound("footstep_near");
            break;
        case 1:
            statusLine = "A whisper threads the hallway; you can't tell if it's a voice or the pipes.";
            panic += 0.04f;
            PlaySound("whisper_far");
            break;
        case 2:
            statusLine = "Silence; your own breath roars loud enough to drown thought.";
            panic += 0.02f;
            PlaySound("silence_hum");
            break;
        case 3:
            statusLine = "A long, deliberate inhale outside; something seems to be searching for rhythm.";
            panic += 0.08f;
            PlaySound("inhale_close");
            break;
        default:
            statusLine = "A soft thud against the wall — was that a foot or your eyes playing tricks?";
            panic += 0.05f;
            PlaySound("thud");
            break;
    }
    statusTimer = 2.6f;
    panic = std::min(1.0f, panic);
}

void SaraBreathingClosetPuzzle::Update(float dt, const InputContext& input) {
    if (input.IsActionPressed(InputAction::PuzzleReset)) { Start(); return; }
    if (solved) return;

    hideTimer -= dt;
    eventTimer -= dt;
    statusTimer = std::max(0.0f, statusTimer - dt);

    // choices mapping
    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedChoice = 0; // hold breath
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedChoice = 1; // peek
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedChoice = 2; // stay still
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedChoice = 3; // shift

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        // apply choice effects
        switch (selectedChoice) {
            case 0: // hold breath
                panic = std::max(0.0f, panic - 0.12f);
                statusLine = "You clamp your breath like a fist. The air trembles, but you do not.";
                PlaySound("breath_hold");
                break;
            case 1: // peek
                panic = std::min(1.0f, panic + 0.18f);
                statusLine = "You risk a crack of vision — a shadow moves and your stomach drops.";
                PlaySound("peek_");
                break;
            case 2: // stay still
                panic = std::max(0.0f, panic - 0.06f);
                statusLine = "You press every muscle into silence. The closet smells like old cloth.";
                PlaySound("shift_small");
                break;
            case 3: // shift
                panic = std::min(1.0f, panic + 0.12f);
                statusLine = "You move slightly; wood sighs. Something pauses outside like a drawn breath.";
                PlaySound("shift_big");
                break;
        }
        statusTimer = 2.4f;
    }

    if (eventTimer <= 0.0f) {
        eventTimer = 3.5f + static_cast<float>(rand() % 40) / 10.0f; // varied events
        TriggerSoundEvent();
    }

    // panic slowly decays if steady
    panic = std::max(0.0f, panic - dt * 0.03f);

    // hallucination mechanic: sometimes events don't correspond to anyone
    if ((rand() % 100) < 6) {
        // subtle hallucination; increase doubt but not panic
        statusLine += "  Was that even real?";
        statusTimer = 1.6f;
    }

    // failure if panic too high
    if (panic >= 0.95f) {
        solved = true;
        solvedMessage = "They found you. Your breath betrays you and something cold presses your shoulder.";
        statusLine = "The closet opens and a light cuts your face. Your hands shake uncontrollably.";
        statusTimer = 4.0f;
        PlaySound("caught");
        return;
    }

    if (hideTimer <= 0.0f) {
        solved = true;
        solvedMessage = "You breathe when the room has moved on. For a moment, it is only you and the closet.";
        statusLine = "Silence loosens; you step out with shaking knees.";
        statusTimer = 3.0f;
        PlaySound("puzzle_complete");
    }
}

void SaraBreathingClosetPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.9f, 0.8f, 0.75f);
    const glm::vec3 warnColor(1.0f, 0.7f, 0.5f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.76f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 36.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 68.0f, 0.34f, glm::vec3(0.85f,0.85f,0.86f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("Sounds: footsteps / drawers / whispers", 72.0f, topY + 110.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);

    // heart and breath (use simple ASCII heart to avoid encoding issues)
    const std::string heart = std::string(static_cast<int>(1 + panic * 8.0f), '*');
    textRenderer.RenderText("Heartbeat: " + heart, 72.0f, topY + 148.0f, 0.36f, glm::vec3(1.0f,0.5f,0.5f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("[1] Hold breath  [2] Peek  [3] Stay still  [4] Shift", 72.0f, static_cast<float>(screenHeight) - 72.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 110.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.44f, glm::vec3(0.95f,0.84f,0.75f) * alpha, screenWidth, screenHeight);
    }
}

std::string SaraBreathingClosetPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << hideTimer << ';' << eventTimer << ';' << panic << ';' << selectedChoice;
    return stream.str();
}

void SaraBreathingClosetPuzzle::DeserializeState(const std::string& state) {
    std::stringstream ss(state);
    std::string token;
    std::getline(ss, token, ';'); solved = (token == "1");
    std::getline(ss, token, ';'); hideTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); eventTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); panic = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); selectedChoice = token.empty() ? 0 : std::stoi(token);
}

void SaraBreathingClosetPuzzle::Reset() {
    hideTimer = 0.0f;
    eventTimer = 0.0f;
    panic = 0.0f;
    selectedChoice = 0;
    solved = false;
    statusLine.clear();
    statusTimer = 0.0f;
}
