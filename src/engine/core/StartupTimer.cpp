#include "engine/core/StartupTimer.h"
#include <iostream>

std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> StartupTimer::s_timers;

void StartupTimer::Begin(const std::string& blockName) {
    s_timers[blockName] = std::chrono::high_resolution_clock::now();
}

void StartupTimer::End(const std::string& blockName) {
    auto it = s_timers.find(blockName);
    if (it != s_timers.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - it->second).count();
        std::cout << "[Timing] " << blockName << " took " << duration << " ms\n";
        s_timers.erase(it);
    }
}
