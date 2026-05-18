#include "game/puzzles/logic/Jade/JadeDebatePuzzle.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

namespace {
using Turn = JadeDebatePuzzle::Exchange;

const std::array<Turn, 4> kTurns = {{
    {
        "You never saw the road.",
        "You saw the road twice.",
        "He watches to see which memory you protect.",
        0,
        {{
            {"I remember the road clearly.", "He scowls. 'You held the line.'", "He grins. 'Good. Let it crack.'", 0.0f, 24.0f},
            {"The road never moved.", "He narrows his eyes. 'You are still pretending.'", "He smirks. 'That hesitation helps me.'", 0.0f, 0.0f},
            {"You are bluffing.", "He snaps. 'No. You are the one slipping.'", "He laughs. 'Yes, keep fighting the wrong battle.'", 8.0f, 0.0f},
            {"Say it again.", "He bites back a grin. 'You want the wound reopened.'", "He beams. 'Now you are bleeding for it.'", 6.0f, 0.0f}
        }}
    },
    {
        "The town chose you.",
        "The town forced itself to notice you.",
        "He leans forward, waiting to see if you flinch.",
        1,
        {{
            {"You are afraid of the witnesses.", "He exhales through his teeth. 'You still missed the point.'", "He smirks. 'Good. Keep doubting.'", 0.0f, 0.0f},
            {"The town forced itself to notice you.", "He scowls. 'Correct. That should have hurt more.'", "He bares his teeth. 'You are making me work for it.'", 0.0f, 24.0f},
            {"Then you were already lying.", "He tilts his head. 'You have no idea how deep this goes.'", "He smiles. 'Keep missing and I win faster.'", 7.0f, 0.0f},
            {"I'm listening. Keep going.", "He frowns. 'Listening is not the same as seeing.'", "He grins. 'That will cost you soon.'", 5.0f, 0.0f}
        }}
    },
    {
        "I was alone in the hall.",
        "Three witnesses heard the knocking.",
        "He taps the table as if counting your heartbeat.",
        1,
        {{
            {"You were alone in the hall.", "He blinks, annoyed. 'You reached for the wrong edge.'", "He smirks. 'Not enough. You have to break deeper.'", 0.0f, 0.0f},
            {"Three witnesses heard the knocking.", "He snarls. 'That is the one thing I could not hide.'", "He looks offended. 'You are pulling the room away from me.'", 0.0f, 24.0f},
            {"The knocking was yours.", "He laughs once, cold and sharp. 'No. Yours is the easier lie.'", "He smiles wider. 'Good. You are helping me.'", 9.0f, 0.0f},
            {"The hall remembers you.", "He stiffens. 'It remembers what I let it keep.'", "He grins. 'That answer feeds me.'", 6.0f, 0.0f}
        }}
    },
    {
        "Nothing was taken.",
        "Someone left with your name.",
        "He is almost smiling now, certain you will miss the last cut.",
        3,
        {{
            {"Nothing was taken.", "He looks irritated. 'You are still circling the wound.'", "He smirks. 'Good. Stay shallow.'", 0.0f, 0.0f},
            {"I took it back.", "He squints. 'No. You do not get to reclaim it that easily.'", "He grins. 'That answer makes you easy to break.'", 7.0f, 0.0f},
            {"The name was a decoy.", "He huffs. 'You are close, and still not close enough.'", "He smiles. 'Yes. Keep missing.'", 5.0f, 0.0f},
            {"Someone left with your name.", "He recoils, angry. 'That is exactly what happened.'", "He scowls. 'You are not supposed to hold that together.'", 0.0f, 24.0f}
        }}
    }
}};

const char* MoodText(JadeDebatePuzzle::Mood mood) {
    switch (mood) {
        case JadeDebatePuzzle::Mood::Smug: return "EMOTION: SMUG";
        case JadeDebatePuzzle::Mood::Angry: return "EMOTION: ANGRY";
        case JadeDebatePuzzle::Mood::Neutral: default: return "EMOTION: WATCHING";
    }
}

glm::vec3 MoodColor(JadeDebatePuzzle::Mood mood) {
    switch (mood) {
        case JadeDebatePuzzle::Mood::Smug: return glm::vec3(1.0f, 0.86f, 0.28f);
        case JadeDebatePuzzle::Mood::Angry: return glm::vec3(0.96f, 0.46f, 0.22f);
        case JadeDebatePuzzle::Mood::Neutral: default: return glm::vec3(0.94f, 0.94f, 0.88f);
    }
}

const char* PhaseName(JadeDebatePuzzle::Phase phase) {
    switch (phase) {
        case JadeDebatePuzzle::Phase::Opening: return "OPENING PRESSURE";
        case JadeDebatePuzzle::Phase::CrossExamination: return "CROSS-EXAMINATION";
        case JadeDebatePuzzle::Phase::Pressure: return "MANIPULATION PRESSURE";
        case JadeDebatePuzzle::Phase::Verdict: return "VERDICT";
    }
    return "UNKNOWN";
}
}

void JadeDebatePuzzle::Start() {
    Reset();
    active = true;
    phase = Phase::Opening;
    yellowLine = "The Man in Yellow smiles. 'You will remember this wrong.'";
    responseLine = "Choose the lie to break first.";
    emotionLine = MoodText(mood);
    promptTimer = 2.4f;
    overlay.Trigger(1.0f);
    atmosphere.Pulse(18.0f);
    distortion.SetAmount(0.15f);
    PlaySound("puzzle_start");
}

void JadeDebatePuzzle::AdvancePrompt() {
    const Exchange& exchange = kTurns[static_cast<std::size_t>(currentPrompt)];
    yellowLine = (contradictionsExposed % 2 == 0) ? exchange.lie : exchange.truth;
    selectedAnswer = 0;
    mood = Mood::Neutral;
    responseLine = exchange.leadIn;
    emotionLine = MoodText(mood);
    promptTimer = 2.8f;
    dialoguePulse = 1.0f;
    tracker.Record({"ManInYellow", yellowLine, static_cast<int>(timer * 1000.0f)});
}

void JadeDebatePuzzle::EscalatePressure(float amount) {
    sanity.AddPressure(amount);
    atmosphere.Pulse(amount * 0.75f);
    overlay.Trigger(0.12f + amount * 0.02f);
    distortion.SetAmount(std::min(1.0f, distortion.GetAmount() + amount * 0.01f));
    pressureBursts++;
}

void JadeDebatePuzzle::ExposeContradiction(int index) {
    if (!active || solved) {
        return;
    }

    const Exchange& exchange = kTurns[static_cast<std::size_t>(currentPrompt)];
    if (index < 0 || index >= static_cast<int>(exchange.choices.size())) {
        return;
    }

    const Choice& choice = exchange.choices[static_cast<std::size_t>(index)];
    mood = (choice.sanityLoss > 0.0f) ? Mood::Smug : Mood::Angry;
    emotionLine = MoodText(mood);

    if (choice.sanityLoss > 0.0f) {
        responseLine = choice.smugResponse;
        EscalatePressure(choice.sanityLoss + 6.0f + static_cast<float>(pressureBursts) * 1.5f);
        PlaySound("puzzle_fail");
    } else {
        responseLine = choice.angryResponse;
        overlay.Trigger(0.24f);
        atmosphere.Pulse(4.0f);
        PlaySound("puzzle_tick");
    }

    if (sanity.IsBroken()) {
        active = false;
        solved = false;
        corrupted = true;
        phase = Phase::Verdict;
        mood = Mood::Angry;
        emotionLine = "EMOTION: CORRUPTED";
        responseLine = "You are dead. Corrupted.";
        yellowLine = "The room keeps talking after you stop answering.";
        solvedMessage = "You are dead. Corrupted.";
        PlaySound("puzzle_fail");
        return;
    }

    if (choice.truthGain > 0.0f) {
        contradictionsExposed++;
        truth.AddTruth(choice.truthGain);
        overlay.Trigger(0.35f);
        atmosphere.Pulse(8.0f);

        if (contradictionsExposed >= 4) {
            solved = true;
            phase = Phase::Verdict;
            solvedMessage = "The Man in Yellow collapses under the weight of his own lies.";
            responseLine = "He is angry now. The room finally hears what he kept hiding.";
            emotionLine = MoodText(Mood::Angry);
            PlaySound("puzzle_complete");
            return;
        }

        phase = Phase::Pressure;
        pressureTimer = 1.5f;
        currentPrompt = (currentPrompt + 1) % static_cast<int>(kTurns.size());
    }
}

void JadeDebatePuzzle::Update(float dt, const InputContext& input) {
    if (!active) {
        return;
    }

    timer += dt;
    promptTimer = std::max(0.0f, promptTimer - dt);
    pressureTimer = std::max(0.0f, pressureTimer - dt);
    dialoguePulse = std::max(0.0f, dialoguePulse - dt * 1.25f);

    overlay.Update(dt);
    atmosphere.Escalate(dt);
    sanity.Update(dt);

    if (sanity.IsBroken()) {
        active = false;
        solved = false;
        corrupted = true;
        phase = Phase::Verdict;
        mood = Mood::Angry;
        emotionLine = "EMOTION: CORRUPTED";
        responseLine = "You are dead. Corrupted.";
        yellowLine = "The room overwhelms the argument. Sanity breaks first.";
        solvedMessage = "You are dead. Corrupted.";
        PlaySound("puzzle_fail");
        return;
    }

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (phase == Phase::Opening && promptTimer <= 0.0f) {
        phase = Phase::CrossExamination;
        AdvancePrompt();
    }

    if (phase == Phase::CrossExamination || phase == Phase::Pressure) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedAnswer = 0;
        if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedAnswer = 1;
        if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedAnswer = 2;
        if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedAnswer = 3;

        if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
            ExposeContradiction(selectedAnswer);
        }

        if (input.IsActionPressed(InputAction::PuzzleCancel)) {
            responseLine = "You hesitate. He leans into the silence and feeds it back.";
            EscalatePressure(4.0f);
            PlaySound("puzzle_tick");
        }
    }

    if (phase == Phase::Pressure && pressureTimer <= 0.0f && !solved) {
        phase = Phase::CrossExamination;
        AdvancePrompt();
    }

    if (!solved && truth.GetTruth() >= 100.0f) {
        solved = true;
        phase = Phase::Verdict;
        solvedMessage = "Truth overwhelms the lie. The Man in Yellow has nowhere left to hide.";
        responseLine = "The debate becomes a reckoning.";
        PlaySound("puzzle_complete");
    }
}

void JadeDebatePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float topY = static_cast<float>(screenHeight) * 0.16f;
    const glm::vec3 yellow(1.0f, 0.88f, 0.35f);
    const glm::vec3 rust(1.0f, 0.72f, 0.18f);
    const glm::vec3 white(0.94f, 0.94f, 0.88f);
    const glm::vec3 orange(1.0f, 0.76f, 0.2f);

    textRenderer.RenderText(GetTitle(), centerX - 260.0f, topY, 1.00f, yellow * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("PSYCHOLOGICAL CONFRONTATION", centerX - 206.0f, topY + 42.0f, 0.56f, orange * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(GetClueText(), centerX - 290.0f, topY + 82.0f, 0.46f, white * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("PHASE: " + std::string(PhaseName(phase)), centerX - 150.0f, topY + 128.0f, 0.54f, orange * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(emotionLine.empty() ? MoodText(mood) : emotionLine, centerX - 120.0f, topY + 166.0f, 0.50f, MoodColor(mood) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("TRUTH", 72.0f, static_cast<float>(screenHeight) - 168.0f, 0.56f, white * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("[" + std::string(static_cast<int>(truth.GetTruth() / 10.0f), '#') + std::string(10 - static_cast<int>(truth.GetTruth() / 10.0f), '-') + "]",
                           72.0f, static_cast<float>(screenHeight) - 136.0f, 0.54f, yellow * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("SANITY", 72.0f, static_cast<float>(screenHeight) - 100.0f, 0.56f, white * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("[" + std::string(static_cast<int>(sanity.GetSanity() / 10.0f), '#') + std::string(10 - static_cast<int>(sanity.GetSanity() / 10.0f), '-') + "]",
                           72.0f, static_cast<float>(screenHeight) - 74.0f, 0.54f, orange * alpha, screenWidth, screenHeight);

    textRenderer.RenderText(yellowLine, 72.0f, topY + 172.0f, 0.54f, white * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(responseLine, 72.0f, topY + 216.0f, 0.48f, yellow * alpha, screenWidth, screenHeight);

    const Exchange& exchange = kTurns[static_cast<std::size_t>(currentPrompt)];
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedAnswer == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : white;
        const std::string option = std::to_string(i + 1) + ". " + exchange.choices[static_cast<std::size_t>(i)].text;
        textRenderer.RenderText(option, 72.0f, topY + 258.0f + static_cast<float>(i) * 38.0f, 0.46f, color * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE MAN IN YELLOW RUNS OUT OF MASKS", centerX - 320.0f, static_cast<float>(screenHeight) * 0.82f, 0.54f, yellow * alpha, screenWidth, screenHeight);
    }

    if (corrupted) {
        textRenderer.RenderText("YOU ARE DEAD. CORRUPTED.", centerX - 240.0f, static_cast<float>(screenHeight) * 0.82f, 0.72f, glm::vec3(1.0f, 0.16f, 0.16f) * alpha, screenWidth, screenHeight);
    }

    const std::string footer = corrupted
        ? "[L] close fail state"
        : "[1-4] choose contradiction  [ENTER] challenge  [L] resist  [R] restart";
    textRenderer.RenderText(footer,
                           72.0f, static_cast<float>(screenHeight) - 42.0f, corrupted ? 0.48f : 0.42f, white * alpha, screenWidth, screenHeight);
}

std::string JadeDebatePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (active ? 1 : 0) << ';' << (solved ? 1 : 0) << ';' << static_cast<int>(phase) << ';'
           << currentPrompt << ';' << selectedAnswer << ';' << contradictionsExposed << ';' << pressureBursts << ';'
           << timer << ';' << promptTimer << ';' << pressureTimer << ';' << dialoguePulse << ';' << static_cast<int>(mood) << ';' << (corrupted ? 1 : 0);
    return stream.str();
}

void JadeDebatePuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); active = (token == "1");
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); phase = static_cast<Phase>(token.empty() ? 0 : std::stoi(token));
    std::getline(stream, token, ';'); currentPrompt = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); selectedAnswer = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); contradictionsExposed = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); pressureBursts = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); promptTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); pressureTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); dialoguePulse = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); mood = token.empty() ? Mood::Neutral : static_cast<Mood>(std::stoi(token));
    std::getline(stream, token, ';'); corrupted = (token == "1");
}

void JadeDebatePuzzle::Reset() {
    phase = Phase::Opening;
    currentPrompt = 0;
    selectedAnswer = 0;
    contradictionsExposed = 0;
    pressureBursts = 0;
    active = false;
    solved = false;
    mood = Mood::Neutral;
    yellowLine.clear();
    responseLine.clear();
    emotionLine.clear();
    timer = 0.0f;
    promptTimer = 0.0f;
    pressureTimer = 0.0f;
    dialoguePulse = 0.0f;
    corrupted = false;
    sanity = SanitySystem();
    truth = TruthSystem();
    tracker = ContradictionTracker();
    overlay = HallucinationOverlay();
    atmosphere = AtmosphereSystem();
    distortion = AudioDistortion();
}
