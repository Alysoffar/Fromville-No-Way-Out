#pragma once

#include <algorithm>
#include <array>
#include <string>

struct RuntimeProfileFrame {
    float inputPhaseMs = 0.0f;
    float eventPhaseMs = 0.0f;
    float gameplayPhaseMs = 0.0f;
    float entityPhaseMs = 0.0f;
    float worldPhaseMs = 0.0f;
    float interactionPhaseMs = 0.0f;
    float puzzlePhaseMs = 0.0f;
    float uiPhaseMs = 0.0f;
    float totalMs = 0.0f;
};

class RuntimeProfiler {
public:
    void AddFrame(const RuntimeProfileFrame& frame) {
        history[writeIndex] = frame;
        writeIndex = (writeIndex + 1) % history.size();
        count = std::min(count + 1, static_cast<int>(history.size()));
    }

    RuntimeProfileFrame GetLatest() const {
        if (count <= 0) {
            return RuntimeProfileFrame{};
        }

        int idx = writeIndex - 1;
        if (idx < 0) {
            idx += static_cast<int>(history.size());
        }
        return history[static_cast<std::size_t>(idx)];
    }

private:
    std::array<RuntimeProfileFrame, 120> history{};
    int writeIndex = 0;
    int count = 0;
};
