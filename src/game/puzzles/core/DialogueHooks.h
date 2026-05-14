#pragma once

#include <functional>
#include <string>
#include <nlohmann/json.hpp>

class DialogueHooks {
public:
    using Callback = std::function<void(const std::string&)>;
    void RegisterOnLine(const Callback& cb) { onLine = cb; }
    void TriggerLine(const std::string& line) { if (onLine) onLine(line); }
    nlohmann::json Serialize() const { return nlohmann::json::object(); }
    void Deserialize(const nlohmann::json&) {}
private:
    Callback onLine;
};
