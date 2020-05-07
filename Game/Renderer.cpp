#include "pch.h"

#include "Renderer.h"
#include "Common.h"
#include "File.h"
#include "Transform.h"
#include "Camera.h"
#include "ArrayView.h"
#include "RendererHelpers.h"
#include "Buffer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <stdexcept>
#include <array>
#include <dxgi.h>

enum RenderTargetFlags : u32
{
    RT_Color = (1 << 0),
    RT_Depth = (1 << 1),
    RT_DepthSRV = (1 << 2),
};

class RenderTarget
{
public:
    ComPtr<ID3D11Texture2D> m_framebuffer;
    ComPtr<ID3D11Texture2D> m_depthTexture;

    ComPtr<ID3D11RenderTargetView> m_framebufferRTV;
    ComPtr<ID3D11ShaderResourceView> m_framebufferSRV;

    ComPtr<ID3D11DepthStencilView> m_dsv;
    ComPtr<ID3D11ShaderResourceView> m_depthSRV;

    u32 m_width;
    u32 m_height;

    void init(const ComPtr<ID3D11Device1>& device, u32 width, u32 height, u32 flags,
        DXGI_FORMAT colorFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthTextureFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthSRVFormat = DXGI_FORMAT_UNKNOWN)
    {
        m_width = width;
        m_height = height;

        if (flags & RT_Color) {
            m_framebuffer = createTexture2D(device, colorFormat, m_width, m_height, 1, 1,
                D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
            m_framebufferRTV = createRenderTargetView(device, m_framebuffer.Get());
            m_framebufferSRV = createShaderResourceView(device, m_framebuffer.Get(), D3D11_SRV_DIMENSION_TEXTURE2D,
                colorFormat);
        }

        if (flags & RT_Depth) {
            UINT depthFlags = D3D11_BIND_DEPTH_STENCIL;

            if (flags & RT_DepthSRV) {
                depthFlags |= D3D11_BIND_SHADER_RESOURCE;
            }

            m_depthTexture = createTexture2D(device, depthTextureFormat, m_width, m_height, 1, 1, depthFlags);
            m_dsv = createDepthStencilView(device, m_depthTexture.Get(), D3D11_DSV_DIMENSION_TEXTURE2D, depthFormat);

            if (flags & RT_DepthSRV) {
                m_depthSRV = createShaderResourceView(device, m_depthTexture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, depthSRVFormat);
            }
        }
    }
};

class Renderable
{
public:
    VertexBuffer<Vertex> m_vertexBuffer;
    IndexBuffer<u16> m_indexBuffer;
    ConstantBuffer<RenderableConstantBuffer> m_constantBuffer;
};

class Renderer : public IRenderer
{
public:
    Renderer(SDL_Window* window);

    virtual Renderable* createRenderable(ArrayView<Vertex> vertices, ArrayView<u16> indices) override;

    virtual void setDirectionalLight(const XMFLOAT3& pos) override;
    virtual void setPointLights(ArrayView<PointLight> lights) override;
    virtual void draw(Renderable*, const Camera&, const struct Transform&) override;
    virtual void debugDraw(const Camera&, ArrayView<DebugDrawVertex>) override;
    virtual void clear(float r, float g, float b) override;

    virtual void beginFrame() override;
    virtual void endFrame() override;

    virtual void fullScreenPass() override;

    virtual void beginShadowPass() override;
    virtual void drawShadow(Renderable*, const Camera&, const struct Transform&) override;
    virtual void endShadowPass() override;

private:
    void loadShaders();

    ComPtr<ID3D11VertexShader> loadVertexShader(const char* path, std::function<void(const std::vector<u8>&)> callback = nullptr);
    ComPtr<ID3D11PixelShader> loadPixelShader(const char* path);

    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_shadowWidth = 1024;
    UINT m_shadowHeight = 1024;

    std::vector<std::unique_ptr<Renderable>> m_renderables;

    ComPtr<IDXGISwapChain1> m_swapChain;

    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;

    ComPtr<ID3D11VertexShader> m_fsvs;
    ComPtr<ID3D11PixelShader> m_fsps;

    ComPtr<ID3D11InputLayout> m_debugInputLayout;
    ComPtr<ID3D11VertexShader> m_debugVS;
    ComPtr<ID3D11PixelShader> m_debugPS;
    VertexBuffer<DebugDrawVertex> m_debugVertexBuffer;

    ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;

    ConstantBuffer<CameraConstantBuffer> m_cameraConstantBuffer;
    ConstantBuffer<CameraConstantBuffer> m_shadowCameraConstantBuffer;
    ConstantBuffer<PSConstantBuffer> m_psConstants;

    StructuredBuffer<PointLight> m_pointLights;
    ComPtr<ID3D11ShaderResourceView> m_pointLightBufferSRV;

    ComPtr<ID3D11RasterizerState> m_shadowRasterizerState;
    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11VertexShader> m_shadowVS;
    ComPtr<ID3D11PixelShader> m_shadowPS;

    ComPtr<ID3D11SamplerState> m_framebufferSampler;

    RenderTarget m_shadowRT;
    RenderTarget m_mainRT;
};

using namespace DirectX;

static HWND getWindowHandle(SDL_Window* window)
{
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(window, &wmi);

    return wmi.info.win.window;
}

Renderer::Renderer(SDL_Window* window)
{
    UINT devFlags = 0;
    if constexpr (IsDebug) {
        devFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    std::array featureLevels{ D3D_FEATURE_LEVEL_11_1 };

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    m_width = static_cast<UINT>(w);
    m_height = static_cast<UINT>(h);

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    D3D_FEATURE_LEVEL supportedLevel;
    Hresult hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, devFlags,
        featureLevels.data(), static_cast<UINT>(featureLevels.size()), D3D11_SDK_VERSION, &device, &supportedLevel, &context);

    hr = device->QueryInterface(m_device.GetAddressOf());
    hr = context->QueryInterface(m_context.GetAddressOf());

    ComPtr<IDXGIFactory2> dxgiFactory;
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        hr = device->QueryInterface<IDXGIDevice>(&dxgiDevice);

        ComPtr<IDXGIAdapter> adapter;
        hr = dxgiDevice->GetAdapter(&adapter);
        hr = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(dxgiFactory.GetAddressOf()));
    }

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = m_width;
    sd.Height = m_height;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferCount = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    auto hwnd = getWindowHandle(window);
    hr = dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &sd, nullptr, nullptr, &m_swapChain);

    ComPtr<ID3D11Texture2D> backbuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()));

    m_backbufferRTV = createRenderTargetView(m_device, backbuffer.Get());

    loadShaders();

    m_cameraConstantBuffer.init(m_device);
    m_shadowCameraConstantBuffer.init(m_device);

    {
        auto dirV = XMVector3Normalize(XMVectorSet(1.0f, -1.0f, 0.0f, 0.0f));

        XMStoreFloat3(&m_psConstants.data.LightPosition, dirV);
        m_psConstants.data.NumPointLights = 0;

        m_psConstants.init(m_device);
    }

    {
        m_shadowRT.init(m_device, m_shadowWidth, m_shadowHeight, RT_Depth | RT_DepthSRV, DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);

        m_shadowSampler = createSamplerState(m_device, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_LESS, nullptr, 0.0f, D3D11_FLOAT32_MAX);

        m_shadowRasterizerState = createRasterizerState(m_device, D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, 0, 0.0f, 0.0f, TRUE,
            FALSE, FALSE, FALSE);
    }

    {
        m_depthStencilState = createDepthStencilState(m_device,
            TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_GREATER,
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    {
        m_framebufferSampler = createSamplerState(m_device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_NEVER, nullptr, 0.0f, D3D11_FLOAT32_MAX);

        m_mainRT.init(m_device, m_width, m_height, RT_Color | RT_Depth | RT_DepthSRV, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);
    }
}

Renderable* Renderer::createRenderable(ArrayView<Vertex> vertices, ArrayView<u16> indices)
{
    auto renderable = std::make_unique<Renderable>();
    
    renderable->m_vertexBuffer.init(m_device, vertices);
    renderable->m_indexBuffer.init(m_device, indices);
    renderable->m_constantBuffer.init(m_device);

    return m_renderables.emplace_back(std::move(renderable)).get();
}

void Renderer::setDirectionalLight(const XMFLOAT3& pos)
{
    auto dir = XMVector3Normalize(XMLoadFloat3(&pos));
    XMStoreFloat3(&m_psConstants.data.LightPosition, dir);

    m_psConstants.update(m_context);
}

void Renderer::setPointLights(ArrayView<PointLight> lights)
{
    if (lights.size > m_pointLights.getCapacity()) {
        m_pointLights.init(m_device, lights.size);
        m_pointLightBufferSRV = createShaderResourceView(m_device, m_pointLights.getBuffer(),
            m_pointLights.getBuffer(), DXGI_FORMAT_UNKNOWN, 0, m_pointLights.getCapacity());
    }

    m_pointLights.update(m_context, lights);
    m_psConstants.data.NumPointLights = lights.size;
    m_psConstants.update(m_context);
}

void Renderer::draw(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    {
        m_cameraConstantBuffer.data.ViewMatrix = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.ProjectionMatrix = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    {
        auto wm = transform.getMatrix();
        renderable->m_constantBuffer.data.WorldMatrix = XMMatrixTranspose(wm);
        renderable->m_constantBuffer.data.WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm);
        renderable->m_constantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.bufferGet(), renderable->m_constantBuffer.bufferGet(),
        m_shadowCameraConstantBuffer.bufferGet() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.bufferGet(), m_psConstants.bufferGet() };
    std::array psResources{ m_pointLightBufferSRV.Get(), m_shadowRT.m_depthSRV.Get() };
    std::array psSamplers{ m_shadowSampler.Get() };

    std::array vertexBuffers{ renderable->m_vertexBuffer.getBuffer() };
    std::array strides{ static_cast<UINT>(sizeof(Vertex)) };
    std::array offsets{ UINT(0) };

    m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(renderable->m_indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());

    m_context->VSSetShader(m_vs.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->PSSetShader(m_ps.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(0, static_cast<UINT>(psConstantBuffers.size()), psConstantBuffers.data());
    m_context->PSSetShaderResources(0, static_cast<UINT>(psResources.size()), psResources.data());
    m_context->PSSetSamplers(0, static_cast<UINT>(psSamplers.size()), psSamplers.data());

    m_context->DrawIndexed(renderable->m_indexBuffer.getSize(), 0, 0);
}

void Renderer::debugDraw(const Camera& camera, ArrayView<DebugDrawVertex> vertices)
{
    if (vertices.size > m_debugVertexBuffer.getCapacity()) {
        m_debugVertexBuffer.init(m_device, vertices);
    } else {
        m_debugVertexBuffer.update(m_context, vertices);
    }

    {
        m_cameraConstantBuffer.data.ViewMatrix = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.ProjectionMatrix = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.bufferGet() };
    std::array vertexBuffers{ m_debugVertexBuffer.getBuffer() };
    std::array strides{ static_cast<UINT>(sizeof(DebugDrawVertex)) };
    std::array offsets{ UINT(0) };

    m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    m_context->IASetInputLayout(m_debugInputLayout.Get());

    m_context->VSSetShader(m_debugVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->PSSetShader(m_debugPS.Get(), nullptr, 0);

    m_context->Draw(vertices.size, 0);
}

void Renderer::clear(float r, float g, float b)
{
    float color[4] = { r, g, b, 1.0f };
    m_context->ClearRenderTargetView(m_mainRT.m_framebufferRTV.Get(), color);
    m_context->ClearDepthStencilView(m_mainRT.m_dsv.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
}

void Renderer::beginFrame()
{
    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_mainRT.m_framebufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_mainRT.m_dsv.Get());
}

void Renderer::endFrame()
{
    m_swapChain->Present(0, 0);
}

void Renderer::fullScreenPass()
{
    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);

    m_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_context->VSSetShader(m_fsvs.Get(), nullptr, 0);
    m_context->PSSetShader(m_fsps.Get(), nullptr, 0);

    std::array psSamplers{ m_framebufferSampler.Get() };
    std::array psResources{ m_mainRT.m_framebufferSRV.Get(), m_mainRT.m_depthSRV.Get() };

    m_context->PSSetShaderResources(0, static_cast<UINT>(psResources.size()), psResources.data());
    m_context->PSSetSamplers(0, static_cast<UINT>(psSamplers.size()), psSamplers.data());

    m_context->Draw(4, 0);
}

void Renderer::beginShadowPass()
{
    // Make sure the shadow stuff isn't bound
    std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> srvs;
    std::memset(srvs.data(), 0, sizeof(srvs));
    m_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs.data());

    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_shadowWidth), static_cast<float>(m_shadowHeight));
    m_context->RSSetViewports(1, &vp);

    m_context->ClearDepthStencilView(m_shadowRT.m_dsv.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
    m_context->OMSetRenderTargets(0, nullptr, m_shadowRT.m_dsv.Get());

    m_context->PSSetShader(nullptr, nullptr, 0);
    m_context->RSSetState(m_shadowRasterizerState.Get());
}

void Renderer::drawShadow(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    {
        m_cameraConstantBuffer.data.ViewMatrix = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.ProjectionMatrix = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);

        m_shadowCameraConstantBuffer.data = m_cameraConstantBuffer.data;
        m_shadowCameraConstantBuffer.update(m_context);
    }

    {
        auto wm = transform.getMatrix();

        renderable->m_constantBuffer.data.WorldMatrix = XMMatrixTranspose(wm);
        renderable->m_constantBuffer.data.WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm);
        renderable->m_constantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.bufferGet(), renderable->m_constantBuffer.bufferGet() };

    std::array vertexBuffers{ renderable->m_vertexBuffer.getBuffer() };
    std::array strides{ static_cast<UINT>(sizeof(Vertex)) };
    std::array offsets{ UINT(0) };

    m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(renderable->m_indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());

    m_context->VSSetShader(m_shadowVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->DrawIndexed(renderable->m_indexBuffer.getSize(), 0, 0);
}

void Renderer::endShadowPass()
{
    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_context->RSSetState(nullptr);
}

void Renderer::loadShaders()
{
    m_ps = loadPixelShader("shaders/PixelShader.ps.cso");
    m_vs = loadVertexShader("shaders/VertexShader.vs.cso",
        [this](const std::vector<u8>& bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode.data(), bytecode.size(), &m_inputLayout);
        });

    m_shadowVS = loadVertexShader("shaders/Shadow.vs.cso");

    m_debugPS = loadPixelShader("shaders/DebugDraw.ps.cso");
    m_debugVS = loadVertexShader("shaders/DebugDraw.vs.cso",
        [this](const std::vector<u8>& bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode.data(), bytecode.size(), &m_debugInputLayout);
        });

    m_fsvs = loadVertexShader("shaders/FullScreenPass.vs.cso");
    m_fsps = loadPixelShader("shaders/FullScreenPass.ps.cso");
}

ComPtr<ID3D11VertexShader> Renderer::loadVertexShader(const char* path, std::function<void(const std::vector<u8>&)> callback)
{
    auto bytecode = loadFile(path);

    ComPtr<ID3D11VertexShader> shader;
    Hresult hr = m_device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &shader);

    if (callback) {
        callback(bytecode);
    }

    return shader;
}

ComPtr<ID3D11PixelShader> Renderer::loadPixelShader(const char* path)
{
    auto bytecode = loadFile(path);

    ComPtr<ID3D11PixelShader> shader;
    Hresult hr = m_device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &shader);

    return shader;
}

std::unique_ptr<IRenderer> createRenderer(SDL_Window* window)
{
    return std::unique_ptr<IRenderer>(new Renderer(window));
}
