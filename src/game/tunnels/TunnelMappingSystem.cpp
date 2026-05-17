#include "game/tunnels/TunnelMappingSystem.h"

#include <algorithm>
#include <random>

namespace {
static TunnelMappingSystem* g_tunnelSystem = nullptr;
}

TunnelMappingSystem& TunnelMappingSystem::Instance() {
    if (!g_tunnelSystem) {
        g_tunnelSystem = new TunnelMappingSystem();
    }
    return *g_tunnelSystem;
}

TunnelMappingSystem::TunnelMappingSystem()
    : mappingProgress(0.0f), discoveredNodes(0), atCentralChamber(false) {
    // Preregister standard tunnel nodes
    TunnelNode north{"North Passage", glm::vec3(-1.0f, -1.0f, 0.0f), TunnelTerrain::Stone, 140, "n-o-o-o"};
    TunnelNode east{"East Chamber", glm::vec3(1.5f, 0.0f, 0.0f), TunnelTerrain::Resonant, 165, "e-e-e"};
    TunnelNode south{"South Flooded", glm::vec3(0.0f, 1.5f, 0.0f), TunnelTerrain::Flooded, 110, "s-s-hhhh"};
    TunnelNode west{"West Shrine", glm::vec3(-1.5f, 0.5f, 0.0f), TunnelTerrain::Ritual, 185, "w-i-i-ine"};
    TunnelNode center{"Central Altar", glm::vec3(0.0f, 0.0f, 0.0f), TunnelTerrain::Ritual, 175, "mmmmm-crack"};

    RegisterNode(north);
    RegisterNode(east);
    RegisterNode(south);
    RegisterNode(west);
    RegisterNode(center);

    // Define valid routes
    RegisterRoute({{"North Passage", "East Chamber", "Central Altar"}, "Direct path through resonance chamber"});
    RegisterRoute({{"West Shrine", "South Flooded", "Central Altar"}, "Ritual route through water"});
}

void TunnelMappingSystem::Update(float dt) {
    if (floodingIntensity > 0.0f) {
        floodingIntensity = std::max(0.0f, floodingIntensity - dt * 0.5f);
    }

    if (shiftIntensity > 0.0f) {
        shiftIntensity = std::max(0.0f, shiftIntensity - dt * 0.6f);
    }

    if (echoDistortion > 0.0f) {
        echoDistortion = std::max(0.0f, echoDistortion - dt * 0.8f);
    }
}

void TunnelMappingSystem::Render() const {
    // Rendered by map overlay system
}

void TunnelMappingSystem::RegisterNode(const TunnelNode& node) {
    allNodes.push_back(node);
}

void TunnelMappingSystem::RegisterRoute(const TunnelRoute& route) {
    std::string routeKey;
    for (const auto& name : route.nodeNames) {
        routeKey += name + "->";
    }
    routes[routeKey] = route;
}

void TunnelMappingSystem::TriangulateFromEcho(const std::vector<std::string>& heardEchoes, float accuracy) {
    // Validate if the triangulated nodes form a valid route
    if (ValidateRoute(heardEchoes)) {
        mappingProgress = std::min(1.0f, mappingProgress + 0.25f * accuracy);
    } else {
        mappingProgress = std::max(0.0f, mappingProgress - 0.1f);
    }
}

void TunnelMappingSystem::PlayAudioPulse(const std::string& nodeName, float intensity) {
    for (auto& node : allNodes) {
        if (node.name == nodeName) {
            node.discovered = true;
            DiscoverNode(nodeName);
            break;
        }
    }
}

bool TunnelMappingSystem::ValidateRoute(const std::vector<std::string>& route) {
    std::string routeKey;
    for (const auto& name : route) {
        routeKey += name + "->";
    }

    auto it = routes.find(routeKey);
    if (it != routes.end() && !it->second.isFake) {
        return true;
    }

    // Check if route leads to central chamber
    if (!route.empty() && route.back() == "Central Altar") {
        return true;
    }

    return false;
}

void TunnelMappingSystem::DiscoverNode(const std::string& nodeName) {
    for (auto& node : allNodes) {
        if (node.name == nodeName && !node.discovered) {
            node.discovered = true;
            node.discoveryOrder = discoveredNodes++;
            discoveredNodesList.push_back(node);
            mappingProgress = std::min(1.0f, mappingProgress + 0.15f);
            break;
        }
    }
}

void TunnelMappingSystem::FloatThroughWater(float duration) {
    floodingIntensity = 1.0f;
    mappingProgress = std::min(1.0f, mappingProgress + 0.1f);
}

bool TunnelMappingSystem::IsFlooded(const std::string& nodeName) const {
    for (const auto& node : allNodes) {
        if (node.name == nodeName && node.terrain == TunnelTerrain::Flooded) {
            return true;
        }
    }
    return false;
}

void TunnelMappingSystem::TriggerTunnelShift() {
    shiftIntensity = 1.0f;
    // Shuffle tunnel connections
    static std::mt19937 rng(std::random_device{}());
    std::shuffle(allNodes.begin(), allNodes.end(), rng);
}

void TunnelMappingSystem::CreateFakeTunnel(const std::string& fakePath) {
    fakeTunnels.push_back(fakePath);
}

void TunnelMappingSystem::PlayDistortedEcho() {
    echoDistortion = 1.0f;
}

void TunnelMappingSystem::ReachCentralChamber() {
    atCentralChamber = true;
    mappingProgress = 1.0f;
}
