#include "game/puzzles/logic/Tabitha/TabithaWhisperRoutePuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

TabithaWhisperRoutePuzzle::TabithaWhisperRoutePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {}

void TabithaWhisperRoutePuzzle::Start() {
    Reset();
    // Build nodes: each represents an intersection with a whisper cue and options
    nodes.clear();
    nodes.push_back({
        "A low vaulted junction, damp air presses small and close.",
        "...anghkooey...",
        "The whisper comes again, soft and breathy: 'anghkooey...' — like a plea.",
        "soft tapping from the wall",
        1,
        0,
        {"A quiet warning — avoid ahead", "Move quickly — run now", "Hide here — safety close", "Search for child drawings"},
        false, false, false
    });

    nodes.push_back({
        "A narrow corridor littered with child's chalked hands on the wall.",
        "...anghkooey... anghkooey...",
        "The voices thread together near the wall; whisper overlaps, like memory repeating.",
        "chalked handprints and a crude house drawing",
        0,
        3,
        {"A memory — inspect drawings", "The voices are lost — proceed", "This is a trap — do not enter", "There is a hiding place here"},
        false, false, false
    });

    nodes.push_back({
        "The tunnel forks near a flooded arch; the water gurgles like a throat.",
        "ANGHKOOEY!",
        "The whisper sharpens into a staccato tapping and then a shout: 'ANGHKOOEY!' — urgent, panicked.",
        "rapid tapping echoes from the left tunnel",
        2,
        1,
        {"Retreat — the route is unsafe", "Run — immediate danger ahead", "Quietly bypass", "Call out to the children"},
        false, false, false
    });

    nodes.push_back({
        "A small alcove with a faint glow of a lantern; the air smells of old fruit.",
        "...anghkooey...",
        "Several overlapping voices hum the word, softer now, like lullaby rhythm.",
        "a faded child's drawing of three figures",
        0,
        2,
        {"This is safe — rest here", "Search for clues", "The voice is deceiving — be wary", "The children point to a hidden door"},
        false, false, false
    });

    currentNode = 0;
    selectedInterpretation = -1;
    ambientTension = 0.0f;
    solved = false;
    failed = false;
    statusLine = "The children cannot speak; they speak in the way the tunnel feels.";
    statusTimer = 3.2f;
    PlaySound("puzzle_open");
}

void TabithaWhisperRoutePuzzle::ApplyInterpretation(int choice) {
    if (choice < 0 || choice >= 4) return;
    Node& node = nodes[static_cast<std::size_t>(currentNode)];
    if (choice == node.correctInterpretation) {
        // success at this node
        statusLine = "Your interpretation echoes true; the children quiet with relief.";
        statusTimer = 3.0f;
        ambientTension = std::max(0.0f, ambientTension - 0.12f);
        node.revealed = true;
        PlaySound("whisper_ack");
        // record pattern observation
        AddJournalEntry("Pattern observed at: " + node.locationDesc + " — interpretation matches the children's tone.");
        // advance or try to progress phase
        if (currentNode + 1 < static_cast<int>(nodes.size())) {
            currentNode++;
            selectedInterpretation = -1;
            // reveal next whisper line as the player moves
            statusLine = "A new whisper threads from the dark ahead.";
            statusTimer = 2.6f;
        }
        TryAdvancePhase();
    } else {
        // wrong interpretation: increase tension and possibly trigger failure
        ambientTension = std::min(1.0f, ambientTension + 0.2f);
        statusLine = "The children wail — your meaning is lost. The tunnel grows crueler.";
        statusTimer = 3.4f;
        PlaySound("whisper_wrong");
        // consequences depending on hazard level
        const int hazard = node.hazardLevel;
        if (hazard >= 2 && ambientTension > 0.6f) {
            // immediate failure: monster encounter
            failed = true;
            solved = true; // mark as finished
            solvedMessage = "A skittering thing emerges from the dark; you are forced into a panic and run — the route collapses.";
            PlaySound("monster_claw");
        } else if (hazard == 1 && ambientTension > 0.7f) {
            // false chamber / hallucination
            failed = true;
            solved = true;
            solvedMessage = "The tunnel folds inward; memory and stone mingle and you find only echoes where a path should be.";
            PlaySound("hallucination_swirl");
        }
    }
}

void TabithaWhisperRoutePuzzle::Update(float dt, const InputContext& input) {
    if (input.IsActionPressed(InputAction::PuzzleReset)) { Start(); return; }
    if (solved) return;

    statusTimer = std::max(0.0f, statusTimer - dt);

    // Movement between nodes
    if (input.IsActionPressed(InputAction::PuzzleLeft)) MoveNode(-1);
    if (input.IsActionPressed(InputAction::PuzzleRight)) MoveNode(1);

    // Listen (Q) and Inspect (E) actions
    if (input.IsActionPressed(InputAction::PuzzleDecrease)) {
        ListenNode();
    }
    if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
        InspectNode();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedInterpretation = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedInterpretation = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedInterpretation = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedInterpretation = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        if (selectedInterpretation >= 0) {
            ApplyInterpretation(selectedInterpretation);
        } else {
            statusLine = "You hesitate; the whispers won't wait.";
            statusTimer = 2.0f;
        }
    }

    // toggle journal with option5
    if (input.IsActionPressed(InputAction::PuzzleOption5)) {
        journalVisible = !journalVisible;
    }

    // ambient tension increases slightly over time in dangerous stretches
    ambientTension = std::min(1.0f, ambientTension + dt * (nodes[static_cast<std::size_t>(currentNode)].hazardLevel * 0.02f));

    // subtle chance of layered whispers growing if tension high
    if ((rand() % 100) < static_cast<int>(ambientTension * 25.0f)) {
        statusLine = "The whispers layer together — a chorus of small voices grows nearer.";
        statusTimer = 2.2f;
    }
}

void TabithaWhisperRoutePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.9f, 0.85f, 0.8f);
    const glm::vec3 cueColor(0.85f, 0.8f, 0.78f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, cueColor * alpha, screenWidth, screenHeight);

    const Node& node = nodes[static_cast<std::size_t>(currentNode)];

    textRenderer.RenderText(node.locationDesc, 72.0f, topY + 86.0f, 0.36f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(node.whisperBase, 72.0f, topY + 128.0f, 0.34f, glm::vec3(0.95f,0.9f,0.85f) * alpha, screenWidth, screenHeight);
    // ambient cue (tapping / crying)
    textRenderer.RenderText(node.ambientCue, 72.0f, topY + 156.0f, 0.30f, glm::vec3(0.86f,0.8f,0.78f) * alpha, screenWidth, screenHeight);

    // options
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedInterpretation == i) ? glm::vec3(1.0f,0.9f,0.75f) : glm::vec3(0.92f,0.9f,0.88f);
        textRenderer.RenderText("[" + std::to_string(i + 1) + "] " + node.options[static_cast<std::size_t>(i)], 72.0f, static_cast<float>(screenHeight) - 72.0f + static_cast<float>(i) * -28.0f, 0.32f, color * alpha, screenWidth, screenHeight);
    }

    // status
    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 210.0f, 0.34f, glm::vec3(0.95f,0.85f,0.8f) * alpha, screenWidth, screenHeight);
    }

    // Journal toggle indicator
    textRenderer.RenderText("Journal entries: " + std::to_string(journal.size()) + "  (Press [5] to toggle)", static_cast<float>(screenWidth) - 520.0f, topY + 34.0f, 0.32f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);

    // Render journal if visible
    if (journalVisible) {
        float jy = topY + 72.0f;
        textRenderer.RenderText("--- Investigation Journal ---", static_cast<float>(screenWidth) - 520.0f, jy, 0.34f, glm::vec3(0.95f,0.9f,0.85f) * alpha, screenWidth, screenHeight);
        jy += 28.0f;
        int count = 0;
        for (auto it = journal.rbegin(); it != journal.rend() && count < 8; ++it, ++count) {
            textRenderer.RenderText(*it, static_cast<float>(screenWidth) - 520.0f, jy, 0.30f, glm::vec3(0.85f) * alpha, screenWidth, screenHeight);
            jy += 22.0f;
        }
    }

    // ambient tension bar
    const float barY = static_cast<float>(screenHeight) * 0.5f;
    std::string tension = "Tension: ";
    int ticks = static_cast<int>(ambientTension * 10.0f);
    for (int i = 0; i < 10; ++i) tension += (i < ticks) ? "#" : "-";
    textRenderer.RenderText(tension, 72.0f, barY, 0.34f, glm::vec3(1.0f,0.6f,0.6f) * alpha, screenWidth, screenHeight);

    if (solved && !failed) {
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.42f, glm::vec3(0.8f,0.95f,0.9f) * alpha, screenWidth, screenHeight);
    } else if (solved && failed) {
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.42f, glm::vec3(0.95f,0.75f,0.7f) * alpha, screenWidth, screenHeight);
    }
}

std::string TabithaWhisperRoutePuzzle::SerializeState() const {
    std::ostringstream ss;
    ss << (solved ? 1 : 0) << ';' << static_cast<int>(phase) << ';' << currentNode << ';' << selectedInterpretation << ';' << ambientTension << ';' << (failed ? 1 : 0) << ';' << evidenceCount;
    ss << ';' << journal.size() << ';';
    for (const auto& e : journal) {
        ss << e << "<ENTRY>";
    }
    return ss.str();
}

void TabithaWhisperRoutePuzzle::DeserializeState(const std::string& state) {
    std::stringstream ss(state);
    std::string token;
    std::getline(ss, token, ';'); solved = (token == "1");
    std::getline(ss, token, ';'); phase = token.empty() ? Phase::InitialContact : static_cast<Phase>(std::stoi(token));
    std::getline(ss, token, ';'); currentNode = token.empty() ? 0 : std::stoi(token);
    std::getline(ss, token, ';'); selectedInterpretation = token.empty() ? -1 : std::stoi(token);
    std::getline(ss, token, ';'); ambientTension = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); failed = (token == "1");
    std::getline(ss, token, ';'); evidenceCount = token.empty() ? 0 : std::stoi(token);
    std::getline(ss, token, ';');
    const int journalSize = token.empty() ? 0 : std::stoi(token);
    journal.clear();
    if (journalSize > 0) {
        std::string rest;
        std::getline(ss, rest);
        size_t pos = 0;
        while (true) {
            size_t p = rest.find("<ENTRY>", pos);
            if (p == std::string::npos) break;
            journal.push_back(rest.substr(pos, p - pos));
            pos = p + 7;
        }
    }
}

void TabithaWhisperRoutePuzzle::Reset() {
    currentNode = 0;
    selectedInterpretation = -1;
    ambientTension = 0.0f;
    solved = false;
    failed = false;
    statusLine.clear();
    statusTimer = 0.0f;
    phase = Phase::InitialContact;
    journal.clear();
    journalVisible = false;
    evidenceCount = 0;
    for (auto& n : nodes) {
        n.visited = false;
        n.evidenceCollected = false;
        n.revealed = false;
    }
}

void TabithaWhisperRoutePuzzle::InspectNode() {
    Node& node = nodes[static_cast<std::size_t>(currentNode)];
    if (!node.evidenceCollected) {
        node.evidenceCollected = true;
        evidenceCount++;
        AddJournalEntry(std::string("Found: ") + node.ambientCue + " at " + node.locationDesc);
        statusLine = "You inspect the nearby wall; something small suggests the children's presence.";
        statusTimer = 3.0f;
        PlaySound("inspect_rustle");
        TryAdvancePhase();
    } else {
        statusLine = "You find nothing new upon closer inspection.";
        statusTimer = 1.6f;
    }
}

void TabithaWhisperRoutePuzzle::ListenNode() {
    Node& node = nodes[static_cast<std::size_t>(currentNode)];
    node.visited = true;
    AddJournalEntry(std::string("Heard: ") + node.whisperDetail + " at " + node.locationDesc);
    statusLine = node.whisperDetail;
    statusTimer = 3.0f;
    PlaySound("listen_whisper");
    TryAdvancePhase();
}

void TabithaWhisperRoutePuzzle::MoveNode(int delta) {
    const int count = static_cast<int>(nodes.size());
    currentNode = (currentNode + delta + count) % count;
    selectedInterpretation = -1;
    statusLine = "You move through the tunnels, following the trace of the whisper.";
    statusTimer = 1.6f;
    PlaySound("footstep_soft");
}

void TabithaWhisperRoutePuzzle::AddJournalEntry(const std::string& entry) {
    journal.push_back(entry);
    if (static_cast<int>(journal.size()) > 50) journal.erase(journal.begin());
}

void TabithaWhisperRoutePuzzle::TryAdvancePhase() {
    // simple heuristics to move phases based on exploration and evidence
    int visitedCount = 0;
    for (const auto& n : nodes) if (n.visited) visitedCount++;
    if (phase == Phase::InitialContact && visitedCount >= static_cast<int>(nodes.size())) {
        phase = Phase::PatternRecognition;
        AddJournalEntry("Phase: Pattern recognition — comparing whispers and locations.");
        statusLine = "You begin to notice patterns in tone and place.";
        statusTimer = 3.0f;
        PlaySound("note_record");
        return;
    }
    if (phase == Phase::PatternRecognition && evidenceCount >= 2) {
        phase = Phase::EnvironmentalInvestigation;
        AddJournalEntry("Phase: Environmental investigation — testing reactions.");
        statusLine = "The tunnel itself seems to answer when the word is spoken.";
        statusTimer = 3.0f;
        PlaySound("note_record");
        return;
    }
    if (phase == Phase::EnvironmentalInvestigation && journal.size() >= 5) {
        phase = Phase::EmotionalUnderstanding;
        AddJournalEntry("Phase: Emotional understanding — the word's meaning shifts with feeling.");
        statusLine = "You understand now: the children speak with feeling, not words.";
        statusTimer = 3.0f;
        PlaySound("puzzle_complete");
        // do not mark solved here; leave for the player to confirm via evidence
    }
}
