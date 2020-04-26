#pragma once

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <vector>
#include <memory>

using Microsoft::WRL::ComPtr;

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

struct ConstantBuffer
{
    DirectX::XMMATRIX WorldMatrix;
    DirectX::XMMATRIX ViewMatrix;
    DirectX::XMMATRIX ProjectionMatrix;
};

class Renderable
{
public:
    ~Renderable() = default;

private:
    Renderable() = default;

    friend class Renderer;

    UINT m_vertexCount;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
};

class Renderer
{
public:
    Renderer(SDL_Window* window);

    Renderable* createRenderable(const std::vector<Vertex>& vertices);

    void draw(Renderable*);
    void clear(float r, float g, float b);
    void endFrame();

private:
    std::vector<std::unique_ptr<Renderable>> m_renderables;

    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;

    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;
};
