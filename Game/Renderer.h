#pragma once

#include "Common.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Camera;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

static_assert(sizeof(Vertex) == 40);

struct DebugDrawVertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

static_assert(sizeof(DebugDrawVertex) == 28);

struct CameraConstantBuffer
{
    XMMATRIX ViewMatrix;
    XMMATRIX ProjectionMatrix;
};

struct RenderableConstantBuffer
{
    XMMATRIX WorldMatrix;
    XMMATRIX WorldInvTransposeMatrix;
};

struct alignas(16) PSConstantBuffer
{
    XMFLOAT3 LightPosition;
    UINT NumPointLights;
};

struct PointLight
{
    XMFLOAT4 Position;
    XMFLOAT4 Color;
};

class Renderable
{
public:
    ~Renderable() = default;

private:
    Renderable() = default;

    friend class Renderer;

    UINT m_vertexCount;
    UINT m_indexCount;

    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void setDirectionalLight(const XMFLOAT3& pos) = 0;
    virtual void setPointLights(const std::vector<PointLight>& lights) = 0;
    virtual void draw(Renderable*, const Camera&, const struct Transform&) = 0;
    virtual void debugDraw(const Camera&, const std::vector<DebugDrawVertex>&) = 0;
    virtual void clear(float r, float g, float b) = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void beginShadowPass() = 0;
    virtual void drawShadow(Renderable*, const Camera&, const struct Transform&) = 0;
    virtual void endShadowPass() = 0;

    virtual Renderable* createRenderable(const std::vector<Vertex>& vertices, const std::vector<u16>& indices) = 0;

    ID3D11Device1* getDevice() { return m_device.Get(); }
    ID3D11DeviceContext1* getDeviceContext() { return m_context.Get(); }

protected:
    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;
};

std::unique_ptr<IRenderer> createRenderer(SDL_Window*);

