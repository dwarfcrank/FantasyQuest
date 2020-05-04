#include "pch.h"

#include "Renderer.h"
#include "Common.h"
#include "File.h"
#include "Transform.h"
#include "Camera.h"
#include "ArrayView.h"

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

template<typename T>
struct ConstantBuffer
{
    ComPtr<ID3D11Buffer> buffer;
    T data;

    void update(const ComPtr<ID3D11DeviceContext>& context)
    {
        context->UpdateSubresource(buffer.Get(), 0, nullptr, &data, 0, 0);
    }
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

    template<typename... Args>
    auto createTexture2D(Args... args)
    {
        CD3D11_TEXTURE2D_DESC desc(args...);
        ComPtr<ID3D11Texture2D> texture;
        Hresult hr = m_device->CreateTexture2D(&desc, nullptr, &texture);
        return texture;
    }

    template<typename... Args>
    auto createBuffer(Args... args)
    {
        CD3D11_BUFFER_DESC desc(args...);
        ComPtr<ID3D11Buffer> buffer;
        Hresult hr = m_device->CreateBuffer(&desc, nullptr, &buffer);
        return buffer;
    }

    template<typename T, typename... Args>
    auto createBuffer(ArrayView<T> contents, Args... args)
    {
        CD3D11_BUFFER_DESC desc(contents.byteSize(), args...);
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = contents.data;

        ComPtr<ID3D11Buffer> buffer;
        Hresult hr = m_device->CreateBuffer(&desc, &sd, &buffer);
        return buffer;
    }

    template<typename T, typename... Args>
    auto createBuffer(const T& contents, Args... args)
    {
        static_assert(std::is_standard_layout_v<T>);
        CD3D11_BUFFER_DESC desc(sizeof(T), args...);
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = &contents;

        ComPtr<ID3D11Buffer> buffer;
        Hresult hr = m_device->CreateBuffer(&desc, &sd, &buffer);
        return buffer;
    }

    template<typename... Args>
    auto createRenderTargetView(ID3D11Resource* resource, Args... args)
    {
        CD3D11_RENDER_TARGET_VIEW_DESC desc(args...);
        ComPtr<ID3D11RenderTargetView> rtv;
        Hresult hr = m_device->CreateRenderTargetView(resource, &desc, &rtv);
        return rtv;
    }

    auto createRenderTargetView(ID3D11Resource* resource)
    {
        ComPtr<ID3D11RenderTargetView> rtv;
        Hresult hr = m_device->CreateRenderTargetView(resource, nullptr, &rtv);
        return rtv;
    }

    template<typename... Args>
    auto createShaderResourceView(ID3D11Resource* resource, Args... args)
    {
        CD3D11_SHADER_RESOURCE_VIEW_DESC desc(args...);
        ComPtr<ID3D11ShaderResourceView> srv;
        Hresult hr = m_device->CreateShaderResourceView(resource, &desc, &srv);
        return srv;
    }

    template<typename... Args>
    auto createDepthStencilView(ID3D11Resource* resource, Args... args)
    {
        CD3D11_DEPTH_STENCIL_VIEW_DESC desc(args...);
        ComPtr<ID3D11DepthStencilView> dsv;
        Hresult hr = m_device->CreateDepthStencilView(resource, &desc, &dsv);
        return dsv;
    }

    template<typename... Args>
    auto createDepthStencilState(Args... args)
    {
        CD3D11_DEPTH_STENCIL_DESC desc(args...);
        ComPtr<ID3D11DepthStencilState> dss;
        Hresult hr = m_device->CreateDepthStencilState(&desc, &dss);
        return dss;
    }

    template<typename... Args>
    auto createSamplerState(Args... args)
    {
        CD3D11_SAMPLER_DESC desc(args...);
        ComPtr<ID3D11SamplerState> ss;
        Hresult hr = m_device->CreateSamplerState(&desc, &ss);
        return ss;
    }

    template<typename... Args>
    auto createRasterizerState(Args... args)
    {
        CD3D11_RASTERIZER_DESC desc(args...);
        ComPtr<ID3D11RasterizerState> rs;
        Hresult hr = m_device->CreateRasterizerState(&desc, &rs);
        return rs;
    }

    template<typename T>
    void initConstantBuffer(ConstantBuffer<T>& cb)
    {
        cb.buffer = createBuffer(cb.data, D3D11_BIND_CONSTANT_BUFFER);
    }

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
    ComPtr<ID3D11Buffer> m_debugVertexBuffer;
    UINT m_debugVerticesCapacity = 0;

    ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;

    ConstantBuffer<CameraConstantBuffer> m_cameraConstantBuffer;
    ConstantBuffer<CameraConstantBuffer> m_shadowCameraConstantBuffer;
    ConstantBuffer<PSConstantBuffer> m_psConstants;


    ComPtr<ID3D11Buffer> m_pointLightBuffer;
    ComPtr<ID3D11ShaderResourceView> m_pointLightBufferSRV;
    UINT m_pointLightCapacity = 0;

    ComPtr<ID3D11Texture2D> m_depthTexture;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11ShaderResourceView> m_depthSRV;

    ComPtr<ID3D11RasterizerState> m_shadowRasterizerState;
    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11VertexShader> m_shadowVS;
    ComPtr<ID3D11PixelShader> m_shadowPS;
    ComPtr<ID3D11Texture2D> m_shadowTexture;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV;
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;

    ComPtr<ID3D11SamplerState> m_framebufferSampler;
    ComPtr<ID3D11Texture2D> m_framebuffer;
    ComPtr<ID3D11RenderTargetView> m_framebufferRTV;
    ComPtr<ID3D11ShaderResourceView> m_framebufferSRV;
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

    m_backbufferRTV = createRenderTargetView(backbuffer.Get());

    loadShaders();

    initConstantBuffer(m_cameraConstantBuffer);
    initConstantBuffer(m_shadowCameraConstantBuffer);

    {
        auto dirV = XMVector3Normalize(XMVectorSet(1.0f, -1.0f, 0.0f, 0.0f));

        XMStoreFloat3(&m_psConstants.data.LightPosition, dirV);
        m_psConstants.data.NumPointLights = 0;

        initConstantBuffer(m_psConstants);
    }

    {
        m_depthTexture = createTexture2D(DXGI_FORMAT_R32_TYPELESS, m_width, m_height, 1, 1,
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
        m_depthStencilView = createDepthStencilView(m_depthTexture.Get(), D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
        m_depthSRV = createShaderResourceView(m_depthTexture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
    }

    {
        m_shadowTexture = createTexture2D(DXGI_FORMAT_R32_TYPELESS, m_shadowWidth, m_shadowHeight, 1, 1,
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
        m_shadowDSV = createDepthStencilView(m_shadowTexture.Get(), D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
        m_shadowSRV = createShaderResourceView(m_shadowTexture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D,
            DXGI_FORMAT_R32_FLOAT);

        m_shadowSampler = createSamplerState(D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_LESS, nullptr, 0.0f, D3D11_FLOAT32_MAX);

        m_shadowRasterizerState = createRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, 0, 0.0f, 0.0f, TRUE,
            FALSE, FALSE, FALSE);
    }

    {
        m_depthStencilState = createDepthStencilState(
            TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_GREATER,
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    {
        m_framebuffer = createTexture2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1,
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
        m_framebufferRTV = createRenderTargetView(m_framebuffer.Get());
        m_framebufferSRV = createShaderResourceView(m_framebuffer.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM);

        m_framebufferSampler = createSamplerState(D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_NEVER, nullptr, 0.0f, D3D11_FLOAT32_MAX);
    }
}

Renderable* Renderer::createRenderable(ArrayView<Vertex> vertices, ArrayView<u16> indices)
{
    std::unique_ptr<Renderable> renderable(new Renderable());

    renderable->m_vertexBuffer = createBuffer(vertices, D3D11_BIND_VERTEX_BUFFER);
    renderable->m_vertexCount = vertices.size;

    renderable->m_indexBuffer = createBuffer(indices, D3D11_BIND_INDEX_BUFFER);
    renderable->m_indexCount = indices.size;

    RenderableConstantBuffer cb{
        .WorldMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f),
    };

    renderable->m_constantBuffer = createBuffer(cb, D3D11_BIND_CONSTANT_BUFFER);

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
    m_psConstants.data.NumPointLights = lights.size;

    if (lights.size > m_pointLightCapacity) {
        CD3D11_BUFFER_DESC bd(lights.byteSize(), D3D11_BIND_SHADER_RESOURCE);
        bd.StructureByteStride = sizeof(PointLight);
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        Hresult hr = m_device->CreateBuffer(&bd, nullptr, &m_pointLightBuffer);

        m_pointLightCapacity = lights.size;
        m_pointLightBufferSRV = createShaderResourceView(m_pointLightBuffer.Get(),
            m_pointLightBuffer.Get(), DXGI_FORMAT_UNKNOWN, 0, m_pointLightCapacity);
    }

    CD3D11_BOX box(0, 0, 0, lights.byteSize(), 1, 1);

    m_context->UpdateSubresource(m_pointLightBuffer.Get(), 0, &box, lights.data,
        lights.byteSize(), 0);

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
        RenderableConstantBuffer cb{
            .WorldMatrix = XMMatrixTranspose(wm),
            .WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm),
        };

        m_context->UpdateSubresource(renderable->m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.buffer.Get(), renderable->m_constantBuffer.Get(),
        m_shadowCameraConstantBuffer.buffer.Get() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.buffer.Get(), m_psConstants.buffer.Get() };
    std::array psResources{ m_pointLightBufferSRV.Get(), m_shadowSRV.Get() };
    std::array psSamplers{ m_shadowSampler.Get() };

    std::array vertexBuffers{ renderable->m_vertexBuffer.Get() };
    std::array strides{ static_cast<UINT>(sizeof(Vertex)) };
    std::array offsets{ UINT(0) };

    m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(renderable->m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());

    m_context->VSSetShader(m_vs.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->PSSetShader(m_ps.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(0, static_cast<UINT>(psConstantBuffers.size()), psConstantBuffers.data());
    m_context->PSSetShaderResources(0, static_cast<UINT>(psResources.size()), psResources.data());
    m_context->PSSetSamplers(0, static_cast<UINT>(psSamplers.size()), psSamplers.data());

    m_context->DrawIndexed(renderable->m_indexCount, 0, 0);
}

void Renderer::debugDraw(const Camera& camera, ArrayView<DebugDrawVertex> vertices)
{
    if (vertices.size > m_debugVerticesCapacity) {
        m_debugVertexBuffer = createBuffer(vertices, D3D11_BIND_VERTEX_BUFFER);
        m_debugVerticesCapacity = vertices.size;
    } else {
        CD3D11_BOX box(0, 0, 0, vertices.byteSize(), 1, 1);
        m_context->UpdateSubresource(m_debugVertexBuffer.Get(), 0, &box, vertices.data, vertices.byteSize(), 0);
    }

    {
        m_cameraConstantBuffer.data.ViewMatrix = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.ProjectionMatrix = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.buffer.Get() };
    std::array vertexBuffers{ m_debugVertexBuffer.Get() };
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
    m_context->ClearRenderTargetView(m_framebufferRTV.Get(), color);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
}

void Renderer::beginFrame()
{
    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_framebufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_depthStencilView.Get());
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
    std::array psResources{ m_framebufferSRV.Get(), m_depthSRV.Get() };

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

    m_context->ClearDepthStencilView(m_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
    m_context->OMSetRenderTargets(0, nullptr, m_shadowDSV.Get());

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
        RenderableConstantBuffer cb{
            .WorldMatrix = XMMatrixTranspose(wm),
            .WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm),
        };

        m_context->UpdateSubresource(renderable->m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.buffer.Get(), renderable->m_constantBuffer.Get() };

    std::array vertexBuffers{ renderable->m_vertexBuffer.Get() };
    std::array strides{ static_cast<UINT>(sizeof(Vertex)) };
    std::array offsets{ UINT(0) };

    m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(renderable->m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());

    m_context->VSSetShader(m_shadowVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->DrawIndexed(renderable->m_indexCount, 0, 0);
}

void Renderer::endShadowPass()
{
    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_context->RSSetState(nullptr);
}

void Renderer::loadShaders()
{
    m_ps = loadPixelShader("../x64/Debug/PixelShader.cso");
    m_vs = loadVertexShader("../x64/Debug/VertexShader.cso",
        [this](const std::vector<u8>& bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode.data(), bytecode.size(), &m_inputLayout);
        });

    m_shadowVS = loadVertexShader("../x64/Debug/ShadowVertexShader.cso");

    m_debugPS = loadPixelShader("../x64/Debug/DebugDrawPixelShader.cso");
    m_debugVS = loadVertexShader("../x64/Debug/DebugDrawVertexShader.cso",
        [this](const std::vector<u8>& bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode.data(), bytecode.size(), &m_debugInputLayout);
        });

    m_fsvs = loadVertexShader("../x64/Debug/FullScreenPassVS.cso");
    m_fsps = loadPixelShader("../x64/Debug/FullScreenPassPS.cso");
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
