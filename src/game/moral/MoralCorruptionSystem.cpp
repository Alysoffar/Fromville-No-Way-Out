#include "game/moral/MoralCorruptionSystem.h"

#include <algorithm>
#include <cmath>

namespace {
static MoralCorruptionSystem* g_moralSystem = nullptr;
}

MoralCorruptionSystem& MoralCorruptionSystem::Instance() {
    if (!g_moralSystem) {
        g_moralSystem = new MoralCorruptionSystem();
    }
    return *g_moralSystem;
}

MoralCorruptionSystem::MoralCorruptionSystem()
    : mercyScore(0.0f), vengeanceScore(0.0f), fearPulseIntensity(0.0f) {
    // Preregister key NPCs from Sara's quest
    RegisterNPC("Thomas Reed", 0.1f);  // The torturer — low initial trust
    RegisterNPC("Doctor Marcus", 0.6f);  // Former cult member, sympathetic
    RegisterNPC("Sheriff Grant", 0.3f);  // Complicit but not primary offender
    // Sara-related NPCs removed; keep core NPCs only
}

void MoralCorruptionSystem::Update(float dt) {
    if (fearPulseIntensity > 0.0f) {
        fearPulseIntensity = std::max(0.0f, fearPulseIntensity - dt * 0.5f);
    }

    // Passive corruption from guilt knowledge
    if (corruptionFromOvarhearing > 0.0f) {
        corruptionFromOvarhearing = std::max(0.0f, corruptionFromOvarhearing - dt * 0.2f);
    }
}

void MoralCorruptionSystem::RegisterNPC(const std::string& name, float initialTrust) {
    NPCTrust npc{name, initialTrust, 0.0f, false, ""};
    npcTrustMap[name] = npc;
}

void MoralCorruptionSystem::UpdateNPCTrust(const std::string& name, float delta) {
    auto it = npcTrustMap.find(name);
    if (it != npcTrustMap.end()) {
        it->second.trustLevel = std::clamp(it->second.trustLevel + delta, 0.0f, 1.0f);
    }
}

void MoralCorruptionSystem::SetNPCGuilty(const std::string& name, const std::string& secretGuilt) {
    auto it = npcTrustMap.find(name);
    if (it != npcTrustMap.end()) {
        it->second.isLying = true;
        it->second.secretGuilt = secretGuilt;
        it->second.corruptionLevel = 0.8f;
    }
}

float MoralCorruptionSystem::GetNPCTrust(const std::string& name) const {
    auto it = npcTrustMap.find(name);
    if (it != npcTrustMap.end()) {
        return it->second.trustLevel;
    }
    return 0.5f;
}

bool MoralCorruptionSystem::IsNPCLying(const std::string& name) const {
    auto it = npcTrustMap.find(name);
    if (it != npcTrustMap.end()) {
        return it->second.isLying;
    }
    return false;
}

void MoralCorruptionSystem::RegisterEvidence(const EvidenceItem& evidence) {
    allEvidence.push_back(evidence);
}

void MoralCorruptionSystem::DiscoverEvidence(const std::string& evidenceName) {
    for (auto& evidence : allEvidence) {
        if (evidence.name == evidenceName && !evidence.discovered) {
            evidence.discovered = true;
            discoveredEvidence.push_back(evidence);

            // Moral shifting
            if (evidence.impliesMoral == MoralChoice::Mercy) {
                mercyScore += 0.15f;
            } else if (evidence.impliesMoral == MoralChoice::Revenge) {
                vengeanceScore += 0.15f;
            }
            break;
        }
    }
}

void MoralCorruptionSystem::OverhearConversation(const std::string& speakerName, const std::string& content) {
    // Using Ghost Step, Sara overhears conversations
    UpdateNPCTrust(speakerName, -0.1f);  // Overhearing erodes trust
    corruptionFromOvarhearing += 0.05f;

    // If confession detected
    if (content.find("guilty") != std::string::npos || content.find("sorry") != std::string::npos) {
        mercyScore += 0.1f;
    }
}

void MoralCorruptionSystem::DetectGuilt(const std::string& characterName, float guiltIntensity) {
    auto it = npcTrustMap.find(characterName);
    if (it != npcTrustMap.end()) {
        it->second.corruptionLevel = std::clamp(it->second.corruptionLevel + guiltIntensity, 0.0f, 1.0f);
        fearPulseIntensity = std::max(fearPulseIntensity, guiltIntensity);
    }
}

MoralChoice MoralCorruptionSystem::GetCurrentMoralTendency() const {
    if (mercyScore > vengeanceScore) {
        return MoralChoice::Mercy;
    } else if (vengeanceScore > mercyScore) {
        return MoralChoice::Revenge;
    } else {
        return MoralChoice::Exposure;
    }
}

bool MoralCorruptionSystem::IsSuspicious(const std::string& characterName) const {
    auto it = npcTrustMap.find(characterName);
    if (it != npcTrustMap.end()) {
        return it->second.trustLevel < 0.3f || it->second.isLying;
    }
    return false;
}

void MoralCorruptionSystem::PresentFinalChoice(const MoralCheckpoint& checkpoint) {
    // Display the choice prompt
}

void MoralCorruptionSystem::RecordChoice(MoralChoice choice) {
    choiceHistory.push_back(choice);

    switch (choice) {
        case MoralChoice::Mercy:
            mercyScore += 0.3f;
            vengeanceScore = std::max(0.0f, vengeanceScore - 0.1f);
            break;
        case MoralChoice::Revenge:
            vengeanceScore += 0.3f;
            mercyScore = std::max(0.0f, mercyScore - 0.1f);
            break;
        case MoralChoice::Exposure:
            mercyScore += 0.15f;
            vengeanceScore += 0.15f;
            break;
        case MoralChoice::Sacrifice:
            mercyScore += 0.2f;
            vengeanceScore += 0.2f;
            break;
    }
}

MoralChoice MoralCorruptionSystem::CalculateFinalEnding() const {
    if (mercyScore > vengeanceScore + 0.3f) {
        return MoralChoice::Mercy;
    }
    if (vengeanceScore > mercyScore + 0.3f) {
        return MoralChoice::Revenge;
    }
    if (corruptionFromOvarhearing > 0.4f) {
        return MoralChoice::Exposure;
    }
    return MoralChoice::Sacrifice;
}

std::string MoralCorruptionSystem::GetEndingNarrative(MoralChoice choice) const {
    switch (choice) {
        case MoralChoice::Mercy:
            return "A merciful choice soothes some wounds and opens paths to healing.";
        case MoralChoice::Revenge:
            return "Vengeance escalates the conflict, trading justice for more violence.";
        case MoralChoice::Exposure:
            return "Exposing the truth forces the town to face its past, with uncertain outcomes.";
        case MoralChoice::Sacrifice:
            return "A sacrifice seals the danger but at great personal cost.";
    }
    return "";
}
