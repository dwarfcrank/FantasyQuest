#pragma once

#include "../Common.h"

#include "RenderDevice.h"

#include <DirectXMath.h>
#include <d3d11_4.h>
#include <wrl.h>
#include <vector>

using Microsoft::WRL::ComPtr;

template<typename TShader>
struct ShaderParams
{
    TShader* shader = nullptr;

    std::vector<ID3D11Buffer*> constants;
    std::vector<ID3D11ShaderResourceView*> resources;
    std::vector<ID3D11SamplerState*> samplers;
};

template<>
struct ShaderParams<ID3D11VertexShader>
{
    ID3D11VertexShader* shader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    std::vector<ID3D11Buffer*> constants;
    std::vector<ID3D11ShaderResourceView*> resources;
    std::vector<ID3D11SamplerState*> samplers;
};

struct VertexBufferSet
{
    std::vector<ID3D11Buffer*> buffers;
    std::vector<u32> strides;
    std::vector<u32> offsets;
};

struct PassParams
{
    VertexBufferSet vertexBuffers;
    ID3D11Buffer* indexBuffer = nullptr;
    u32 numIndices = 0;
    u32 numInstances = 0;

    u32 baseIndex = 0;
    u32 baseVertex = 0;

    D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ShaderParams<ID3D11VertexShader> vs;
    ShaderParams<ID3D11GeometryShader> gs;
    ShaderParams<ID3D11PixelShader> ps;
};

class RenderContext
{
public:
    RenderContext(ComPtr<ID3D11DeviceContext1> ctx, const DirectX::XMUINT2& backbufferSize,
        ComPtr<ID3D11RenderTargetView> backbufferRTV) :
        m_context(ctx), m_backbufferSize(backbufferSize), m_backbufferRTV(backbufferRTV)
    {}

    void bindBackbuffer();

    void clearRenderTarget(class RenderTarget*, const DirectX::XMFLOAT4& clearColor = { 0.0f, 0.0f, 0.0f, 1.0f, }, float depth = 0.0f);
    void bindRenderTarget(class RenderTarget*);

    void draw(const PassParams&);

private:
    ComPtr<ID3D11DeviceContext1> m_context;

    DirectX::XMUINT2 m_backbufferSize{ 0, 0 };
    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;
};
