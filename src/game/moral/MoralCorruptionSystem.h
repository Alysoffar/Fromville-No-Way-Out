#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

enum class MoralChoice {
    Mercy,
    Exposure,
    Revenge,
    Sacrifice
};

struct NPCTrust {
    std::string characterName;
    float trustLevel = 0.5f;
    float corruptionLevel = 0.0f;
    bool isLying = false;
    std::string secretGuilt;
};

struct EvidenceItem {
    std::string name;
    std::string description;
    MoralChoice impliesMoral;
    bool discovered = false;
};

struct MoralCheckpoint {
    int choiceNumber;
    std::string scenario;
    std::array<std::string, 4> choiceTexts;
    std::array<MoralChoice, 4> correspondingChoices;
};

class MoralCorruptionSystem {
public:
    static MoralCorruptionSystem& Instance();

    void Update(float dt);

    // NPC tracking
    void RegisterNPC(const std::string& name, float initialTrust = 0.5f);
    void UpdateNPCTrust(const std::string& name, float delta);
    void SetNPCGuilty(const std::string& name, const std::string& secretGuilt);
    float GetNPCTrust(const std::string& name) const;
    bool IsNPCLying(const std::string& name) const;

    // Evidence collection
    void RegisterEvidence(const EvidenceItem& evidence);
    void DiscoverEvidence(const std::string& evidenceName);
    const std::vector<EvidenceItem>& GetAllEvidence() const { return allEvidence; }
    const std::vector<EvidenceItem>& GetDiscoveredEvidence() const { return discoveredEvidence; }

    // Sara's Ghost Step ability integration
    void OverhearConversation(const std::string& speakerName, const std::string& content);
    void DetectGuilt(const std::string& characterName, float guiltIntensity);

    // Moral tracking
    float GetMercyScore() const { return mercyScore; }
    float GetVengeanceScore() const { return vengeanceScore; }
    MoralChoice GetCurrentMoralTendency() const;

    // Fear pulse system
    bool IsSuspicious(const std::string& characterName) const;
    float GetFearPulseIntensity() const { return fearPulseIntensity; }

    // Final choice system
    void PresentFinalChoice(const MoralCheckpoint& checkpoint);
    void RecordChoice(MoralChoice choice);
    const std::vector<MoralChoice>& GetChoiceHistory() const { return choiceHistory; }

    // Ending determination
    MoralChoice CalculateFinalEnding() const;
    std::string GetEndingNarrative(MoralChoice choice) const;

private:
    MoralCorruptionSystem();
    ~MoralCorruptionSystem() = default;

    std::unordered_map<std::string, NPCTrust> npcTrustMap;
    std::vector<EvidenceItem> allEvidence;
    std::vector<EvidenceItem> discoveredEvidence;
    std::vector<MoralChoice> choiceHistory;

    float mercyScore = 0.0f;
    float vengeanceScore = 0.0f;
    float fearPulseIntensity = 0.0f;
    float corruptionFromOvarhearing = 0.0f;

    const std::array<const char*, 4> kGuiltReactions = {{
        "They avoid your gaze.",
        "Their voice cracks slightly.",
        "They know you know.",
        "They smile, but something breaks."
    }};
};
