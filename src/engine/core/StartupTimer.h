#pragma once

#include <string>
#include <chrono>
#include <unordered_map>

class StartupTimer {
public:
    static void Begin(const std::string& blockName);
    static void End(const std::string& blockName);
private:
    static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> s_timers;
};
