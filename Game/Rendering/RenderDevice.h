#pragma once

#include "../Common.h"
#include "../ArrayView.h"

#include <memory>
#include <d3d11_4.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

struct VertexShader
{
    ComPtr<ID3D11VertexShader> shader;
    ComPtr<ID3D11InputLayout> inputLayout;
};

struct PixelShader
{
    ComPtr<ID3D11PixelShader> shader;
};

struct GeometryShader
{
    ComPtr<ID3D11GeometryShader> shader;
};

struct ComputeShader
{
    ComPtr<ID3D11ComputeShader> shader;
};

class Buffer2
{
public:
    Buffer2() = default;

    const u32 stride = 0;
    const u32 capacity = 0;

private:
    const ComPtr<ID3D11Buffer> m_buffer;
    u32 m_size = 0;
};

class RenderDevice
{
public:
    RenderDevice(struct SDL_Window*);
    ~RenderDevice();

    std::shared_ptr<VertexShader> loadVertexShader(ID3DBlob* bytecode, ArrayView<D3D11_INPUT_ELEMENT_DESC> inputLayout);
    std::shared_ptr<PixelShader> loadPixelShader(ID3DBlob* bytecode);
    std::shared_ptr<ComputeShader> loadComputeShader(ID3DBlob* bytecode);
    std::shared_ptr<GeometryShader> loadGeometryShader(ID3DBlob* bytecode);

    class RenderContext* getRenderContext()
    {
        return m_renderContext.get();
    }

private:
    u32 m_width = 0;
    u32 m_height = 0;

    std::unique_ptr<class RenderContext> m_renderContext;

    ComPtr<ID3D11Device1> m_device;
};
