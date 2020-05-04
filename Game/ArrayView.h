#pragma once

#include "Common.h"

#include <array>
#include <vector>

template<typename T>
struct ArrayView
{
    const T* const data = nullptr;
    const u32 size = 0;

    ArrayView(const std::vector<T>& container) :
        data(container.data()), size(static_cast<u32>(container.size()))
    {
    }

    template<size_t N>
    constexpr ArrayView(const std::array<T, N>& container) :
        data(container.data()), size(static_cast<u32>(N))
    {
    }

    constexpr ArrayView(const T* data, u32 size) :
        data(data), size(size)
    {
    }

    constexpr u32 byteSize() const
    {
        return size * sizeof(T);
    }
};

template<typename T>
ArrayView(const T*, u32) -> ArrayView<T>;
