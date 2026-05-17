#include "ContradictionTracker.h"

void ContradictionTracker::Record(const Statement& s) {
    statements.push_back(s);
}

std::vector<std::pair<Statement, Statement>> ContradictionTracker::DetectContradictions() const {
    std::vector<std::pair<Statement, Statement>> result;
    for (size_t i = 0; i < statements.size(); ++i) {
        for (size_t j = i + 1; j < statements.size(); ++j) {
            if (statements[i].text != statements[j].text) {
                // If same speaker says different things, flag as contradiction
                if (statements[i].speaker == statements[j].speaker) {
                    result.emplace_back(statements[i], statements[j]);
                }
            }
        }
    }
    return result;
}

nlohmann::json ContradictionTracker::Serialize() const {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& s : statements) {
        arr.push_back({{"speaker", s.speaker}, {"text", s.text}, {"id", s.id}});
    }
    return nlohmann::json{{"statements", arr}};
}

void ContradictionTracker::Deserialize(const nlohmann::json& j) {
    statements.clear();
    if (!j.contains("statements")) return;
    for (auto& it : j.at("statements")) {
        Statement s;
        s.speaker = it.value("speaker", std::string());
        s.text = it.value("text", std::string());
        s.id = it.value("id", 0);
        statements.push_back(s);
    }
}
