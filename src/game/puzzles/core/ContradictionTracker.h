#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Statement {
    std::string speaker;
    std::string text;
    int id = 0;
};

class ContradictionTracker {
public:
    void Record(const Statement& s);
    std::vector<std::pair<Statement, Statement>> DetectContradictions() const;
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    std::vector<Statement> statements;
};
