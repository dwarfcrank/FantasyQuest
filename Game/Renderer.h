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

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

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
    /*
    XMVECTOR Position;
    XMVECTOR Color;
    */
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

class Camera
{
public:
    XMMATRIX getViewMatrix() const;
    XMMATRIX getProjectionMatrix() const;

    void move(float xOff, float yOff, float zOff);
    void setPosition(float x, float y, float z);

    void setDirection(const XMFLOAT3& dir)
    {
        m_direction = XMLoadFloat3(&dir);
    }

    void invertDirection()
    {
        m_direction = -m_direction;
    }

    void setDirection(float x, float y, float z)
    {
        m_direction = XMVectorSet(x, y, z, 0.0f);
    }

    void makeOrtho()
    {
        m_projectionMatrix = XMMatrixOrthographicLH(20.0f, 20.0f, -10.0f, 20.0f);
    }

private:
    XMVECTOR m_position = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    XMVECTOR m_direction = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    
    XMMATRIX m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 16.0f / 9.0f, 0.01f, 100.0f);
};

class Renderer
{
public:
    Renderer(SDL_Window* window);

    Renderable* createRenderable(const std::vector<Vertex>& vertices, const std::vector<u16>& indices);

    void setDirectionalLight(const XMFLOAT3& pos);
    void setPointLights(const std::vector<PointLight>& lights);
    void draw(Renderable*, const Camera&, const struct Transform&);
    void clear(float r, float g, float b);

    void beginFrame();
    void endFrame();

    void beginShadowPass();
    void drawShadow(Renderable*, const Camera&, const struct Transform&);
    void endShadowPass();

    ID3D11Device1* getDevice() { return m_device.Get(); }
    ID3D11DeviceContext1* getDeviceContext() { return m_context.Get(); }

private:
    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_shadowWidth = 1024;
    UINT m_shadowHeight = 1024;

    std::vector<std::unique_ptr<Renderable>> m_renderables;

    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;

    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;

    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;
    ComPtr<ID3D11Buffer> m_cameraConstantBuffer;
    ComPtr<ID3D11Buffer> m_shadowCameraConstantBuffer;
    ComPtr<ID3D11Buffer> m_psConstantBuffer;
    PSConstantBuffer m_psConstants;
    ComPtr<ID3D11Buffer> m_pointLightBuffer;
    ComPtr<ID3D11ShaderResourceView> m_pointLightBufferSRV;
    UINT m_pointLightCapacity = 0;

    ComPtr<ID3D11Texture2D> m_depthStencilTexture;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11VertexShader> m_shadowVS;
    ComPtr<ID3D11PixelShader> m_shadowPS;
    ComPtr<ID3D11Texture2D> m_shadowTexture;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV;
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;
};
