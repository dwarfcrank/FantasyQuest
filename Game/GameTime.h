#pragma once

#include <chrono>

class GameTime
{
public:
    float update();

    float getDelta() const
    {
        return m_delta;
    }

private:
    std::chrono::high_resolution_clock::time_point m_prev = std::chrono::high_resolution_clock::now();
    float m_delta = 0.0f;
};
