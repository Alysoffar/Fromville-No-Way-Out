#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

enum class TunnelTerrain {
    Stone,          // Normal passage
    Flooded,        // Water-filled
    Collapsed,      // Blocked (requires strength/item)
    Resonant,       // Echo chamber
    Void,           // Empty darkness
    Ritual          // Altar chamber
};

struct TunnelNode {
    std::string name;
    glm::vec3 position;
    TunnelTerrain terrain;
    int frequency = 1;  // Audio frequency signature
    std::string echo;
    bool discovered = false;
    bool mapped = false;
    int discoveryOrder = -1;
};

struct TunnelRoute {
    std::vector<std::string> nodeNames;
    std::string description;
    bool isFake = false;
};

class TunnelMappingSystem {
public:
    static TunnelMappingSystem& Instance();

    void Update(float dt);
    void Render() const;

    // Register tunnel nodes
    void RegisterNode(const TunnelNode& node);
    void RegisterRoute(const TunnelRoute& route);

    // Tabitha's Listen ability
    void TriangulateFromEcho(const std::vector<std::string>& heardEchoes, float accuracy);
    void PlayAudioPulse(const std::string& nodeName, float intensity);
    bool ValidateRoute(const std::vector<std::string>& route);

    // Mapping progress
    float GetMappingProgress() const { return mappingProgress; }
    void DiscoverNode(const std::string& nodeName);
    int GetDiscoveredNodeCount() const { return discoveredNodes; }
    const std::vector<TunnelNode>& GetAllNodes() const { return allNodes; }
    const std::vector<TunnelNode>& GetDiscoveredNodes() const { return discoveredNodesList; }

    // Flooded sections
    void FloatThroughWater(float duration);
    bool IsFlooded(const std::string& nodeName) const;

    // Dynamic tunnel events
    void TriggerTunnelShift();
    void CreateFakeTunnel(const std::string& fakePath);
    void PlayDistortedEcho();

    // Central chamber (final destination)
    bool IsAtCentralChamber() const { return atCentralChamber; }
    void ReachCentralChamber();

private:
    TunnelMappingSystem();
    ~TunnelMappingSystem() = default;

    std::vector<TunnelNode> allNodes;
    std::vector<TunnelNode> discoveredNodesList;
    std::unordered_map<std::string, TunnelRoute> routes;
    std::vector<std::string> fakeTunnels;

    float mappingProgress = 0.0f;
    int discoveredNodes = 0;
    bool atCentralChamber = false;

    float floodingIntensity = 0.0f;
    float shiftIntensity = 0.0f;
    float echoDistortion = 0.0f;

    const std::array<const char*, 6> kTunnelClaustrophobia = {{
        "The air grows thin. Something breathes with you.",
        "The stone hums at a frequency that hurts.",
        "Children's laughter echoes from sealed sections.",
        "The passages remember.",
        "Following the echoes... or leading you astray?",
        "You are not alone in the dark."
    }};
};
