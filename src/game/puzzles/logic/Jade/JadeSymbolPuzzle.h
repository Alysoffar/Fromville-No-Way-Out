#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"
#include "game/symbols/SymbolSystem.h"

class TextRenderer;

class JadeSymbolPuzzle final : public PuzzleBase {
public:
    JadeSymbolPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "False Translation Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    struct Fragment {
        std::string glyph;
        std::string visual; // optional ASCII/art representation
        std::array<std::string, 4> meanings;
        int correct = 0;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

        std::array<Fragment, 4> fragments = {{
                {"Crown glyph", R"(     .-"""-.
     .'  .-.  '.
    /   /   \   \
 |   (  X  )  |
 |    \ | /   |
    \    \_/   /
     '.       .'
         '-.__.-')", {"The town welcomes you", "The town watches you", "The road is open", "The road is gone"}, 1},
                {"Hollow eye", R"(  .----.
 /  .--. \
|  |  |  |
 \  `--' /
    '----')", {"Hide the child", "The child is hidden", "The witness is blind", "The witness is awake"}, 3},
                {"Spiral blood", R"(   @
    @@@
 @@@@@
    @@@
     @)", {"The wound never healed", "The wound is blessed", "The wound is borrowed", "The wound is sealed"}, 0},
                {"Split lantern", R"(   ( )   _  ( )
        \_/ (   )
            (___) ) )", {"Leave the door open", "Open the false door", "Close the last door", "Do not trust the light"}, 3}
        }};

    int currentFragment = 0;
    int selectedMeaning = 0;
    int correctTranslations = 0;
    int wrongTranslations = 0;
    bool solved = false;

    float timer = 0.0f;
    float pressure = 0.0f;
    float hallucination = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void SubmitTranslation(int index);
    void AdvanceFragment();
};