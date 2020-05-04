#pragma once

#include <fmt/core.h>
#include <wrl.h>
#include <stdexcept>

class Hresult
{
public:
    Hresult() = default;

    Hresult(HRESULT hr)
    {
        if (FAILED(hr)) {
            throw std::runtime_error(fmt::format("hr={:x}", hr));
        }
    }

    void operator=(HRESULT hr)
    {
        if (FAILED(hr)) {
            throw std::runtime_error(fmt::format("hr={:x}", hr));
        }
    }
};

