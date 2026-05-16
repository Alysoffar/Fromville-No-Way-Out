#include "DialogueManager.h"

#include "DialogueTypes.h"
#include "game/dialogue/DialogueUI.h"
#include "engine/input/InputAction.h"
#include "engine/input/InputContext.h"
#include "engine/audio/AudioManager.h"
#include <filesystem>

extern bool PlayVoiceLine(AudioManager* audio, const std::string& group, const std::string& rawLine, float gain = 0.85f);

#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

DialogueManager& DialogueManager::Instance() {
    static DialogueManager inst;
    return inst;
}

DialogueManager::DialogueManager() = default;

void DialogueManager::SetAudioManager(AudioManager* audio) {
    audioManager = audio;
}

void DialogueManager::LoadConversationsFromFolder(const std::string& folder) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file()) continue;
            const auto ext = entry.path().extension().string();
            if (ext != ".json") continue;

            std::ifstream in(entry.path());
            if (!in.is_open()) continue;
            nlohmann::json j;
            in >> j;
            Conversation convo;
            convo.id = j.value("id", std::string());
            convo.title = j.value("title", std::string());
            convo.startNode = j.value("startNode", std::string());

            if (j.contains("nodes") && j["nodes"].is_array()) {
                for (const auto& n : j["nodes"]) {
                    DialogueNode node;
                    node.id = n.value("id", std::string());
                    node.speaker = n.value("speaker", std::string());
                    node.text = n.value("text", std::string());
                    node.voiceGroup = n.value("voiceGroup", std::string());
                    node.emotion = static_cast<EmotionState>(n.value("emotion", static_cast<int>(node.emotion)));
                    node.timing = static_cast<DialogueTiming>(n.value("timing", static_cast<int>(node.timing)));
                    node.preDelaySeconds = n.value("preDelaySeconds", node.preDelaySeconds);
                    node.postDelaySeconds = n.value("postDelaySeconds", node.postDelaySeconds);
                    node.revealCharactersPerSecond = n.value("revealCharactersPerSecond", node.revealCharactersPerSecond);
                    node.autoAdvance = n.value("autoAdvance", node.autoAdvance);
                    node.nextNodeId = n.value("nextNodeId", std::string());

                    if (n.contains("choices") && n["choices"].is_array()) {
                        for (const auto& c : n["choices"]) {
                            DialogueChoice ch;
                            ch.id = c.value("id", std::string());
                            ch.text = c.value("text", std::string());
                            ch.nextNodeId = c.value("nextNodeId", std::string());
                            ch.trustDelta = c.value("trustDelta", 0);
                            node.choices.push_back(ch);
                        }
                    }

                    convo.nodes[node.id] = node;
                }
            }

            if (!convo.id.empty()) {
                conversations[convo.id] = std::move(convo);
            }
        }
    } catch (...) {
        // ignore and fallback
    }
}


void DialogueManager::Initialize() {
    ui = std::make_unique<DialogueUI>();
    BuildSampleConversations();
}

void DialogueManager::BuildSampleConversations() {
    auto registerConversation = [this](Conversation conversation) {
        conversations[conversation.id] = std::move(conversation);
    };

    {
        Conversation conversation;
        conversation.id = "boyd_conversation";
        conversation.title = "Boyd";

        DialogueNode start;
        start.id = "start";
        start.speaker = "Boyd";
        start.text = "You hear the radio again?";
        start.emotion = EmotionState::Tense;
        start.timing = DialogueTiming::PauseShort;
        start.postDelaySeconds = 0.8f;
        start.revealCharactersPerSecond = 26.0f;

        DialogueChoice staticChoice;
        staticChoice.id = "static";
        staticChoice.text = "Only static.";
        staticChoice.nextNodeId = "static_reply";
        staticChoice.trustDelta = 1;

        DialogueChoice scaredChoice;
        scaredChoice.id = "scared";
        scaredChoice.text = "People are scared.";
        scaredChoice.nextNodeId = "scared_reply";
        scaredChoice.trustDelta = 1;

        DialogueChoice worriedChoice;
        worriedChoice.id = "worried";
        worriedChoice.text = "You're asking because you're worried.";
        worriedChoice.nextNodeId = "worried_reply";
        worriedChoice.trustDelta = 2;

        start.choices = {staticChoice, scaredChoice, worriedChoice};

        DialogueNode staticReply;
        staticReply.id = "static_reply";
        staticReply.speaker = "Boyd";
        staticReply.text = "Static doesn't usually make people stop talking.";
        staticReply.emotion = EmotionState::Neutral;
        staticReply.timing = DialogueTiming::PauseShort;
        staticReply.postDelaySeconds = 0.7f;
        staticReply.revealCharactersPerSecond = 24.0f;
        staticReply.nextNodeId = "end";
        staticReply.autoAdvance = true;

        DialogueNode scaredReply;
        scaredReply.id = "scared_reply";
        scaredReply.speaker = "Boyd";
        scaredReply.text = "Keep them calm if you can. Don't promise what we can't hold.";
        scaredReply.emotion = EmotionState::Calm;
        scaredReply.timing = DialogueTiming::PauseShort;
        scaredReply.postDelaySeconds = 0.85f;
        scaredReply.revealCharactersPerSecond = 24.0f;
        scaredReply.nextNodeId = "end";
        scaredReply.autoAdvance = true;

        DialogueNode worriedReply;
        worriedReply.id = "worried_reply";
        worriedReply.speaker = "Boyd";
        worriedReply.text = "Maybe. Or maybe I'm counting who tells the truth when it matters.";
        worriedReply.emotion = EmotionState::Tense;
        worriedReply.timing = DialogueTiming::PauseLong;
        worriedReply.postDelaySeconds = 1.1f;
        worriedReply.revealCharactersPerSecond = 22.0f;
        worriedReply.nextNodeId = "end";
        worriedReply.autoAdvance = true;

        DialogueNode end;
        end.id = "end";
        end.speaker = "Boyd";
        end.text = "If you hear anyone panicking, bring it to me quiet. We do not need a crowd around it.";
        end.emotion = EmotionState::Sad;
        end.timing = DialogueTiming::PauseLong;
        end.preDelaySeconds = 0.9f;
        end.postDelaySeconds = 1.0f;
        end.revealCharactersPerSecond = 23.0f;

        conversation.nodes[start.id] = start;
        conversation.nodes[staticReply.id] = staticReply;
        conversation.nodes[scaredReply.id] = scaredReply;
        conversation.nodes[worriedReply.id] = worriedReply;
        conversation.nodes[end.id] = end;
        conversation.startNode = "start";
        registerConversation(std::move(conversation));
    }

    {
        Conversation conversation;
        conversation.id = "jade_conversation";
        conversation.title = "Jade";

        DialogueNode start;
        start.id = "start";
        start.speaker = "Jade";
        start.text = "The symbols are the same. That's the problem.";
        start.emotion = EmotionState::Tense;
        start.timing = DialogueTiming::PauseShort;
        start.postDelaySeconds = 0.7f;
        start.revealCharactersPerSecond = 27.0f;

        DialogueChoice patternChoice;
        patternChoice.id = "pattern";
        patternChoice.text = "You found a pattern?";
        patternChoice.nextNodeId = "pattern_reply";
        patternChoice.trustDelta = 2;

        DialogueChoice dismissedChoice;
        dismissedChoice.id = "dismissed";
        dismissedChoice.text = "People are calling it coincidence.";
        dismissedChoice.nextNodeId = "dismissed_reply";
        dismissedChoice.trustDelta = 0;

        DialogueChoice carefulChoice;
        carefulChoice.id = "careful";
        carefulChoice.text = "Tell me what you do know.";
        carefulChoice.nextNodeId = "careful_reply";
        carefulChoice.trustDelta = 1;

        start.choices = {patternChoice, dismissedChoice, carefulChoice};

        DialogueNode patternReply;
        patternReply.id = "pattern_reply";
        patternReply.speaker = "Jade";
        patternReply.text = "Not a pattern. A map someone keeps trying to hide.";
        patternReply.emotion = EmotionState::Calm;
        patternReply.timing = DialogueTiming::PauseShort;
        patternReply.postDelaySeconds = 0.8f;
        patternReply.revealCharactersPerSecond = 24.0f;
        patternReply.nextNodeId = "end";
        patternReply.autoAdvance = true;

        DialogueNode dismissedReply;
        dismissedReply.id = "dismissed_reply";
        dismissedReply.speaker = "Jade";
        dismissedReply.text = "Coincidence is what people call it when they do not want to read the room.";
        dismissedReply.emotion = EmotionState::Angry;
        dismissedReply.timing = DialogueTiming::PauseLong;
        dismissedReply.postDelaySeconds = 1.0f;
        dismissedReply.revealCharactersPerSecond = 23.0f;
        dismissedReply.nextNodeId = "end";
        dismissedReply.autoAdvance = true;

        DialogueNode carefulReply;
        carefulReply.id = "careful_reply";
        carefulReply.speaker = "Jade";
        carefulReply.text = "Fine. Quietly then. If it hears us, it changes.";
        carefulReply.emotion = EmotionState::Fearful;
        carefulReply.timing = DialogueTiming::PauseShort;
        carefulReply.postDelaySeconds = 0.9f;
        carefulReply.revealCharactersPerSecond = 24.0f;
        carefulReply.nextNodeId = "end";
        carefulReply.autoAdvance = true;

        DialogueNode end;
        end.id = "end";
        end.speaker = "Jade";
        end.text = "Bring me anything with a mark on it. I want the truth before everyone else argues with it.";
        end.emotion = EmotionState::Neutral;
        end.timing = DialogueTiming::PauseLong;
        end.preDelaySeconds = 0.6f;
        end.postDelaySeconds = 1.0f;
        end.revealCharactersPerSecond = 23.0f;

        conversation.nodes[start.id] = start;
        conversation.nodes[patternReply.id] = patternReply;
        conversation.nodes[dismissedReply.id] = dismissedReply;
        conversation.nodes[carefulReply.id] = carefulReply;
        conversation.nodes[end.id] = end;
        conversation.startNode = "start";
        registerConversation(std::move(conversation));
    }

    {
        Conversation conversation;
        conversation.id = "tabitha_conversation";
        conversation.title = "Tabitha";

        DialogueNode start;
        start.id = "start";
        start.speaker = "Tabitha";
        start.text = "The walls are breathing again. Not much. Just enough.";
        start.emotion = EmotionState::Fearful;
        start.timing = DialogueTiming::PauseLong;
        start.preDelaySeconds = 0.7f;
        start.postDelaySeconds = 1.0f;
        start.revealCharactersPerSecond = 23.0f;

        DialogueChoice tunnelChoice;
        tunnelChoice.id = "tunnel";
        tunnelChoice.text = "You found another route?";
        tunnelChoice.nextNodeId = "tunnel_reply";
        tunnelChoice.trustDelta = 1;

        DialogueChoice steadyChoice;
        steadyChoice.id = "steady";
        steadyChoice.text = "Keep breathing. Tell me what changed.";
        steadyChoice.nextNodeId = "steady_reply";
        steadyChoice.trustDelta = 2;

        DialogueChoice hiddenChoice;
        hiddenChoice.id = "hidden";
        hiddenChoice.text = "Something moved under the house.";
        hiddenChoice.nextNodeId = "hidden_reply";
        hiddenChoice.trustDelta = 0;

        start.choices = {tunnelChoice, steadyChoice, hiddenChoice};

        DialogueNode tunnelReply;
        tunnelReply.id = "tunnel_reply";
        tunnelReply.speaker = "Tabitha";
        tunnelReply.text = "A narrow one. Fresh cut stone. Someone wanted it forgotten.";
        tunnelReply.emotion = EmotionState::Neutral;
        tunnelReply.timing = DialogueTiming::PauseShort;
        tunnelReply.postDelaySeconds = 0.8f;
        tunnelReply.revealCharactersPerSecond = 24.0f;
        tunnelReply.nextNodeId = "end";
        tunnelReply.autoAdvance = true;

        DialogueNode steadyReply;
        steadyReply.id = "steady_reply";
        steadyReply.speaker = "Tabitha";
        steadyReply.text = "It stopped when I listened. That is the part I do not like.";
        steadyReply.emotion = EmotionState::Sad;
        steadyReply.timing = DialogueTiming::PauseLong;
        steadyReply.postDelaySeconds = 0.9f;
        steadyReply.revealCharactersPerSecond = 22.0f;
        steadyReply.nextNodeId = "end";
        steadyReply.autoAdvance = true;

        DialogueNode hiddenReply;
        hiddenReply.id = "hidden_reply";
        hiddenReply.speaker = "Tabitha";
        hiddenReply.text = "Then we do not go there alone.";
        hiddenReply.emotion = EmotionState::Calm;
        hiddenReply.timing = DialogueTiming::PauseShort;
        hiddenReply.postDelaySeconds = 0.7f;
        hiddenReply.revealCharactersPerSecond = 24.0f;
        hiddenReply.nextNodeId = "end";
        hiddenReply.autoAdvance = true;

        DialogueNode end;
        end.id = "end";
        end.speaker = "Tabitha";
        end.text = "If the house starts humming, leave the room. I mean that literally.";
        end.emotion = EmotionState::Tense;
        end.timing = DialogueTiming::PauseLong;
        end.preDelaySeconds = 0.8f;
        end.postDelaySeconds = 1.0f;
        end.revealCharactersPerSecond = 23.0f;

        conversation.nodes[start.id] = start;
        conversation.nodes[tunnelReply.id] = tunnelReply;
        conversation.nodes[steadyReply.id] = steadyReply;
        conversation.nodes[hiddenReply.id] = hiddenReply;
        conversation.nodes[end.id] = end;
        conversation.startNode = "start";
        registerConversation(std::move(conversation));
    }

    {
        Conversation conversation;
        conversation.id = "victor_conversation";
        conversation.title = "Victor";

        DialogueNode start;
        start.id = "start";
        start.speaker = "Victor";
        start.text = "I remembered a room. I wish I hadn't.";
        start.emotion = EmotionState::Sad;
        start.timing = DialogueTiming::PauseLong;
        start.preDelaySeconds = 0.8f;
        start.postDelaySeconds = 1.0f;
        start.revealCharactersPerSecond = 22.0f;

        DialogueChoice memoryChoice;
        memoryChoice.id = "memory";
        memoryChoice.text = "What did you see?";
        memoryChoice.nextNodeId = "memory_reply";
        memoryChoice.trustDelta = 1;

        DialogueChoice kindChoice;
        kindChoice.id = "kind";
        kindChoice.text = "You don't have to say it all.";
        kindChoice.nextNodeId = "kind_reply";
        kindChoice.trustDelta = 2;

        DialogueChoice bluntChoice;
        bluntChoice.id = "blunt";
        bluntChoice.text = "Was someone there?";
        bluntChoice.nextNodeId = "blunt_reply";
        bluntChoice.trustDelta = 0;

        start.choices = {memoryChoice, kindChoice, bluntChoice};

        DialogueNode memoryReply;
        memoryReply.id = "memory_reply";
        memoryReply.speaker = "Victor";
        memoryReply.text = "A table. A drawing. And someone pretending not to be afraid.";
        memoryReply.emotion = EmotionState::Tense;
        memoryReply.timing = DialogueTiming::PauseShort;
        memoryReply.postDelaySeconds = 0.8f;
        memoryReply.revealCharactersPerSecond = 23.0f;
        memoryReply.nextNodeId = "end";
        memoryReply.autoAdvance = true;

        DialogueNode kindReply;
        kindReply.id = "kind_reply";
        kindReply.speaker = "Victor";
        kindReply.text = "I know. It still hurts when I stop pretending.";
        kindReply.emotion = EmotionState::Calm;
        kindReply.timing = DialogueTiming::PauseLong;
        kindReply.postDelaySeconds = 0.9f;
        kindReply.revealCharactersPerSecond = 22.0f;
        kindReply.nextNodeId = "end";
        kindReply.autoAdvance = true;

        DialogueNode bluntReply;
        bluntReply.id = "blunt_reply";
        bluntReply.speaker = "Victor";
        bluntReply.text = "Yes. They told me not to look back.";
        bluntReply.emotion = EmotionState::Fearful;
        bluntReply.timing = DialogueTiming::PauseShort;
        bluntReply.postDelaySeconds = 0.7f;
        bluntReply.revealCharactersPerSecond = 24.0f;
        bluntReply.nextNodeId = "end";
        bluntReply.autoAdvance = true;

        DialogueNode end;
        end.id = "end";
        end.speaker = "Victor";
        end.text = "If I remember more later, I will tell you. I just need a minute first.";
        end.emotion = EmotionState::Sad;
        end.timing = DialogueTiming::PauseLong;
        end.preDelaySeconds = 0.9f;
        end.postDelaySeconds = 1.0f;
        end.revealCharactersPerSecond = 22.0f;

        conversation.nodes[start.id] = start;
        conversation.nodes[memoryReply.id] = memoryReply;
        conversation.nodes[kindReply.id] = kindReply;
        conversation.nodes[bluntReply.id] = bluntReply;
        conversation.nodes[end.id] = end;
        conversation.startNode = "start";
        registerConversation(std::move(conversation));
    }
}

bool DialogueManager::StartConversation(const std::string& convoId) {
    if (active) {
        return false;
    }

    const auto it = conversations.find(convoId);
    if (it == conversations.end()) {
        return false;
    }

    activeConvo = &it->second;
    currentNodeId = activeConvo->startNode;
    active = true;

    if (ui) {
        ui->OpenConversation(*activeConvo, currentNodeId);
        if (audioManager) audioManager->PlaySound("cinematic_event_sting", 0.8f);
        const DialogueNode* node = ui->GetCurrentNode();
        if (node && audioManager) {
            PlayVoiceLine(audioManager, node->voiceGroup, node->text, 0.85f);
        }
    }

    return true;
}

void DialogueManager::Update(float dt) {
    if (ui) {
        ui->Update(dt);
    }

    if (active && ui && ui->IsWaitingForInput()) {
        const DialogueNode* node = ui->GetCurrentNode();
        if (node && node->autoAdvance && node->choices.empty() && !node->nextNodeId.empty()) {
            ui->AdvanceToNode(node->nextNodeId);
            return;
        }
    }

    if (!active && ui && !ui->IsOpen()) {
        activeConvo = nullptr;
        currentNodeId.clear();
    }

    // Trust decay and emotion drift: small passive decay over time
    const float decayRatePerSecond = 0.2f; // small subtle decay
    if (decayRatePerSecond > 0.0f) {
        for (auto& kv : relationships) {
            RelationshipState& s = kv.second;
            if (s.trust > 0) {
                s.trust = static_cast<int>(std::max(0.0f, static_cast<float>(s.trust) - decayRatePerSecond * dt));
            } else if (s.trust < 0) {
                s.trust = static_cast<int>(std::min(0.0f, static_cast<float>(s.trust) + decayRatePerSecond * dt));
            }
            // small emotion relaxation toward neutral
            if (s.lastEmotion != EmotionState::Neutral) {
                // simplistic relaxation: after long time, set to neutral (placeholder for more complex state)
                // no-op here; advanced system will adjust based on events
            }
        }
    }
}

int DialogueManager::GetTrust(const std::string& character) const {
    const auto it = relationships.find(character);
    if (it == relationships.end()) return 0;
    return it->second.trust;
}

void DialogueManager::ModifyTrust(const std::string& character, int delta) {
    RelationshipState& s = relationships[character];
    s.trust = std::clamp(s.trust + delta, -100, 100);
}

void DialogueManager::SetTrust(const std::string& character, int value) {
    RelationshipState& s = relationships[character];
    s.trust = std::clamp(value, -100, 100);
}

void DialogueManager::AddMemoryFlag(const std::string& character, const std::string& flag) {
    RelationshipState& s = relationships[character];
    if (std::find(s.memoryFlags.begin(), s.memoryFlags.end(), flag) == s.memoryFlags.end()) {
        s.memoryFlags.push_back(flag);
    }
}

void DialogueManager::RemoveMemoryFlag(const std::string& character, const std::string& flag) {
    RelationshipState& s = relationships[character];
    auto it = std::remove(s.memoryFlags.begin(), s.memoryFlags.end(), flag);
    if (it != s.memoryFlags.end()) s.memoryFlags.erase(it, s.memoryFlags.end());
}

bool DialogueManager::HasMemoryFlag(const std::string& character, const std::string& flag) const {
    const auto it = relationships.find(character);
    if (it == relationships.end()) return false;
    const auto& vec = it->second.memoryFlags;
    return std::find(vec.begin(), vec.end(), flag) != vec.end();
}

void DialogueManager::MarkPromise(const std::string& character, const std::string& promiseId) {
    AddMemoryFlag(character, std::string("promise:") + promiseId);
}

void DialogueManager::BreakPromise(const std::string& character, const std::string& promiseId) {
    // mark as broken and lower trust slightly
    RemoveMemoryFlag(character, std::string("promise:") + promiseId);
    AddMemoryFlag(character, std::string("promise_broken:") + promiseId);
    ModifyTrust(character, -6);
}

void DialogueManager::MarkAccusation(const std::string& character, const std::string& accusationId) {
    AddMemoryFlag(character, std::string("accuse:") + accusationId);
    ModifyTrust(character, -4);
}

const RelationshipState* DialogueManager::GetRelationshipState(const std::string& character) const {
    const auto it = relationships.find(character);
    if (it == relationships.end()) return nullptr;
    return &it->second;
}

std::pair<std::string, RelationshipState*> DialogueManager::GetRelationshipStateIndex(int index) {
    if (relationships.empty() || index < 0) return {std::string(), nullptr};
    int i = 0;
    for (auto& kv : relationships) {
        if (i == index) return {kv.first, &kv.second};
        ++i;
    }
    return {std::string(), nullptr};
}

std::pair<std::string, const RelationshipState*> DialogueManager::GetRelationshipStateIndex(int index) const {
    if (relationships.empty() || index < 0) return {std::string(), nullptr};
    int i = 0;
    for (const auto& kv : relationships) {
        if (i == index) return {kv.first, &kv.second};
        ++i;
    }
    return {std::string(), nullptr};
}

void DialogueManager::HandleInput(const InputContext& input) {
    if (!active || !activeConvo || !ui) {
        return;
    }

    const DialogueNode* node = ui->GetCurrentNode();
    if (!node) {
        CloseConversation();
        return;
    }

    if (input.IsActionPressed(InputAction::DialogueCancel)) {
        CloseConversation();
        return;
    }

    if (input.IsActionPressed(InputAction::DialogueAdvance)) {
        if (!ui->IsWaitingForInput()) {
            ui->RevealFully();
            return;
        }

        if (!node->choices.empty()) {
            SelectChoice(ui->GetSelectedChoiceIndex());
            return;
        }

        if (!node->nextNodeId.empty()) {
            ui->AdvanceToNode(node->nextNodeId);
            return;
        }

        CloseConversation();
        return;
    }

    if (!ui->IsWaitingForInput()) {
        return;
    }

    if (input.IsActionPressed(InputAction::DialogueChoicePrev)) {
        ui->MoveSelection(-1);
    }
    if (input.IsActionPressed(InputAction::DialogueChoiceNext)) {
        ui->MoveSelection(+1);
    }

    if (input.IsActionPressed(InputAction::DialogueChoice1)) {
        SelectChoice(0);
        return;
    }
    if (input.IsActionPressed(InputAction::DialogueChoice2)) {
        SelectChoice(1);
        return;
    }
    if (input.IsActionPressed(InputAction::DialogueChoice3)) {
        SelectChoice(2);
        return;
    }
    if (input.IsActionPressed(InputAction::DialogueChoice4)) {
        SelectChoice(3);
        return;
    }
}

void DialogueManager::SelectChoice(int index) {
    if (!active || !activeConvo || !ui) {
        return;
    }

    const auto it = activeConvo->nodes.find(currentNodeId);
    if (it == activeConvo->nodes.end()) {
        return;
    }

    const DialogueNode& node = it->second;
    if (index < 0 || index >= static_cast<int>(node.choices.size())) {
        return;
    }

    const DialogueChoice& choice = node.choices[index];
    auto& relationship = relationships[activeConvo->id];
    relationship.trust += choice.trustDelta;
    relationship.dialogueHistory.push_back(node.speaker + ": " + choice.text);

    if (!choice.nextNodeId.empty()) {
        currentNodeId = choice.nextNodeId;
        ui->SetSelectedChoiceIndex(index);
        ui->AdvanceToNode(currentNodeId);
        if (audioManager) audioManager->PlaySound("cinematic_event_sting", 0.7f);
        const DialogueNode* nextNode = ui->GetCurrentNode();
        if (nextNode && audioManager) {
            PlayVoiceLine(audioManager, nextNode->voiceGroup, nextNode->text, 0.85f);
        }
    } else {
        CloseConversation();
    }
}

void DialogueManager::CloseConversation() {
    active = false;
    if (ui) {
        ui->Close();
    }
}

void DialogueManager::Render(TextRenderer* hud, int screenWidth, int screenHeight) const {
    if (ui && ui->IsOpen()) {
        ui->Render(hud, screenWidth, screenHeight);
    }
}

std::string DialogueManager::SerializeState() const {
    json root;
    for (const auto& kv : conversations) {
        (void)kv; // conversations metadata not serialized here
    }

    json rels = json::object();
    for (const auto& kv : relationships) {
        json r;
        r["trust"] = kv.second.trust;
        r["memoryFlags"] = kv.second.memoryFlags;
        r["history"] = kv.second.dialogueHistory;
        r["lastEmotion"] = static_cast<int>(kv.second.lastEmotion);
        rels[kv.first] = r;
    }
    root["relationships"] = rels;
    return root.dump();
}

void DialogueManager::DumpStateToFile(const std::string& path) const {
    const std::string payload = SerializeState();
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "[Dialogue] Failed to open debug dump path: " << path << "\n";
        return;
    }
    out << payload;
    out.close();
    std::cout << "[Dialogue] Dumped state to " << path << "\n";
}

void DialogueManager::DeserializeState(const std::string& payload) {
    if (payload.empty()) return;
    try {
        const json root = json::parse(payload);
        if (!root.contains("relationships")) return;
        const json rels = root["relationships"];
        for (auto it = rels.begin(); it != rels.end(); ++it) {
            const std::string name = it.key();
            const json r = it.value();
            RelationshipState& state = relationships[name];
            state.trust = r.value("trust", state.trust);
            state.memoryFlags = r.value("memoryFlags", state.memoryFlags);
            state.dialogueHistory = r.value("history", state.dialogueHistory);
            state.lastEmotion = static_cast<EmotionState>(r.value("lastEmotion", static_cast<int>(state.lastEmotion)));
        }
    } catch (const std::exception& e) {
        std::cerr << "[Dialogue] Failed to parse dialogue state: " << e.what() << '\n';
    }
}
