#pragma once

#include <functional>

class Animator {
public:
    using Easing = std::function<float(float)>;

    static float Linear(float t) { return t; }
};
