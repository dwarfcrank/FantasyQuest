#pragma once

#include "RendererHelpers.h"
#include "ArrayView.h"

#include <d3d11_1.h>
#include <wrl.h>

template<typename T>
class ConstantBuffer
{
public:
    T data;

    void init(const ComPtr<ID3D11Device>& device)
    {
        buffer = createBufferWithData(device, ArrayView(&data, 1), D3D11_BIND_CONSTANT_BUFFER);
    }

    void update(const ComPtr<ID3D11DeviceContext>& context)
    {
        context->UpdateSubresource(buffer.Get(), 0, nullptr, &data, 0, 0);
    }

    auto bufferGet() { return buffer.Get(); }

    void setName(std::string_view name)
    {
        setObjectName(buffer, name);
    }

private:
    ComPtr<ID3D11Buffer> buffer;
};

template<typename T, UINT BindFlags>
class Buffer
{
public:
    using ElementType = T;

    auto getCapacity() const { return m_capacity; }
    auto getSize() const { return m_size; }

    auto getBuffer() { return m_buffer.Get(); }

    void init(const ComPtr<ID3D11Device>& device, UINT capacity)
    {
        this->m_capacity = capacity;
        m_size = 0;

        CD3D11_BUFFER_DESC bd(sizeof(ElementType) * capacity, BindFlags);

        if constexpr ((BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0) {
            bd.StructureByteStride = sizeof(ElementType);
            bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        }

        m_buffer = createBuffer(device, bd);
    }

    void init(const ComPtr<ID3D11Device>& device, ArrayView<T> contents)
    {
        m_capacity = contents.size;
        m_size = contents.size;

        if constexpr ((BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0) {
            m_buffer = createBufferWithData(device, contents, BindFlags,
                D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(ElementType));
        } else {
            m_buffer = createBufferWithData(device, contents, BindFlags);
        }

    }

    void update(const ComPtr<ID3D11DeviceContext>& context, ArrayView<T> contents)
    {
        if (contents.size > m_capacity) {
            throw std::runtime_error(fmt::format("Buffer capacity is {}, need {}!", m_capacity, contents.size));
        }

        m_size = contents.size;
        auto byteSize = static_cast<LONG>(m_size * sizeof(ElementType));

        CD3D11_BOX box(0, 0, 0, byteSize, 1, 1);
        context->UpdateSubresource(m_buffer.Get(), 0, &box, contents.data, 0, 0);
    }

    void setName(std::string_view name)
    {
        setObjectName(m_buffer, name);
    }

private:
    UINT m_size = 0;
    UINT m_capacity = 0;
    ComPtr<ID3D11Buffer> m_buffer;
};

template<typename T>
using VertexBuffer = Buffer<T, D3D11_BIND_VERTEX_BUFFER>;

template<typename T>
using IndexBuffer = Buffer<T, D3D11_BIND_INDEX_BUFFER>;

template<typename T>
using StructuredBuffer = Buffer<T, D3D11_BIND_SHADER_RESOURCE>;
