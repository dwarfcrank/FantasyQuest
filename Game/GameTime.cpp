#include "pch.h"
#include "GameTime.h"

float GameTime::update()
{
    auto t = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::microseconds>(t - m_prev);

    m_delta = static_cast<float>(static_cast<double>(d.count()) / 1'000'000.0);
    m_prev = t;

    return m_delta;
}
