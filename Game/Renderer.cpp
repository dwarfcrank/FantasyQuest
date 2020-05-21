#include "pch.h"

#include "Renderer.h"
#include "Common.h"
#include "File.h"
#include "Transform.h"
#include "Camera.h"
#include "ArrayView.h"
#include "RendererHelpers.h"
#include "Buffer.h"
#include "RenderTarget.h"
#include "Shader.h"

#include <im3d.h>

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
#include <string_view>
#include <imgui_impl_dx11.h>

// ugh I hate wchar
#include <stringapiset.h>

static constexpr auto NUM_BLUR_PASSES = 5;

class Renderable
{
public:
    VertexBuffer<Vertex> m_vertexBuffer;
    IndexBuffer<u16> m_indexBuffer;
    ConstantBuffer<RenderableConstants> m_constantBuffer;
    std::string m_name;
};

CB_STRUCT LuminanceHistogramConstants
{
    uint2 inputSize;
    float minLogLuminance = -10.0f;
    float logLuminanceRange;
    float invLogLuminanceRange;
    float deltaTime = 0.0f;
    float tau = 1.1f;
};

class Renderer : public IRenderer
{
public:
    Renderer(SDL_Window* window);

    virtual Renderable* createRenderable(std::string_view name, ArrayView<Vertex> vertices, ArrayView<u16> indices) override;

    virtual void setDirectionalLight(const XMFLOAT3& pos, const XMFLOAT3& color, float intensity) override;
    virtual void setPointLights(ArrayView<PointLight> lights) override;
    virtual void draw(Renderable*, const Camera&, const struct Transform&) override;
    virtual void draw(const RenderBatch& batch, const Camera&) override;
    virtual void drawIm3d(const Camera&, ArrayView<Im3d::DrawList>) override;
    virtual void clear(float r, float g, float b) override;

    virtual void beginFrame() override;
    virtual void endFrame() override;

    virtual void postProcess(const PostProcessParams&) override;

    virtual void beginShadowPass() override;
    virtual void drawShadow(Renderable*, const Camera&, const struct Transform&) override;
    virtual void endShadowPass() override;

    virtual void draw(const RenderBatch& batch) override;
    virtual void drawShadow(const RenderBatch& batch) override;
    virtual void beginFrame(const Camera&) override;
    virtual void beginShadowPass(const Camera&) override;

    virtual void initImgui() override;
    virtual void drawImgui() override;

private:
    void loadShaders();
    ID3D11ShaderResourceView* computeBloom();

    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;
    ComPtr<ID3DUserDefinedAnnotation> m_annotation;

    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_shadowWidth = 1024;
    UINT m_shadowHeight = 1024;

    std::vector<std::unique_ptr<Renderable>> m_renderables;

    ComPtr<IDXGISwapChain1> m_swapChain;

    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;

    ComPtr<ID3D11InputLayout> m_batchInputLayout;
    ComPtr<ID3D11VertexShader> m_batchVS;
    VertexBuffer<RenderableConstants> m_batchInstanceBuffer;

    ComPtr<ID3D11ComputeShader> m_fscs;
    ComPtr<ID3D11UnorderedAccessView> m_backbufferUAV;

    ComPtr<ID3D11InputLayout> m_im3dLayout;
    ComPtr<ID3D11VertexShader> m_im3dLineVS;
    ComPtr<ID3D11PixelShader> m_im3dLinePS;
    ComPtr<ID3D11GeometryShader> m_im3dLineGS;
    ComPtr<ID3D11VertexShader> m_im3dTriangleVS;
    ComPtr<ID3D11PixelShader> m_im3dTrianglePS;
    VertexBuffer<Im3d::VertexData> m_im3dVertexBuffer;
    ComPtr<ID3D11RasterizerState> m_im3dRasterizerState;
    ComPtr<ID3D11BlendState> m_im3dBlendState;
    ComPtr<ID3D11DepthStencilState> m_im3dDepthStencilState;
    ComPtr<ID3D11DepthStencilState> m_im3dGizmoDepthStencilState;

    ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    ComPtr<ID3D11RenderTargetView> m_backbufferRTV;

    ConstantBuffer<CameraConstants> m_cameraConstantBuffer;
    ConstantBuffer<CameraConstants> m_shadowCameraConstantBuffer;
    ConstantBuffer<PSConstants> m_psConstants;
    ConstantBuffer<PostProcessConstants> m_postProcessConstants;

    StructuredBuffer<PointLight> m_pointLights;
    ComPtr<ID3D11ShaderResourceView> m_pointLightBufferSRV;

    ComPtr<ID3D11RasterizerState> m_shadowRasterizerState;
    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11VertexShader> m_shadowVS;
    ComPtr<ID3D11VertexShader> m_shadowBatchVS;
    ComPtr<ID3D11PixelShader> m_shadowPS;

    RenderTarget m_shadowRT;
    RenderTarget m_mainRT;

    struct
    {
        std::array<RenderTarget, NUM_BLUR_PASSES> RTs;
        std::array<RenderTarget, NUM_BLUR_PASSES> RTs2;
        ComPtr<ID3D11ComputeShader> gaussianCS;
        ConstantBuffer<GaussianConstants> constants;
    } m_blur;

    ComPtr<ID3D11SamplerState> m_framebufferSampler;

    ComPtr<ID3D11ComputeShader> m_luminanceHistogramCS;
    ComPtr<ID3D11ComputeShader> m_luminanceAverageCS;
    ConstantBuffer<LuminanceHistogramConstants> m_luminanceHistogramCB;
    RWByteAddressBuffer<uint> m_luminanceHistogram;
    ComPtr<ID3D11UnorderedAccessView> m_luminanceHistogramUAV;
    ComPtr<ID3D11ShaderResourceView> m_luminanceHistogramSRV;
    RenderTarget m_averageLuminance;
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
    SET_OBJECT_NAME(m_context);
    hr = context->QueryInterface(m_annotation.GetAddressOf());

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
    sd.BufferCount = 2;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    auto hwnd = getWindowHandle(window);
    hr = dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &sd, nullptr, nullptr, &m_swapChain);
    {
        std::string_view name("m_swapChain");
        m_swapChain->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.data());
    }

    ComPtr<ID3D11Texture2D> backbuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()));

    m_backbufferRTV = createRenderTargetView(m_device, backbuffer.Get());
    SET_OBJECT_NAME(m_backbufferRTV);

    m_backbufferUAV = createUnorderedAccessView(m_device, backbuffer.Get());
    SET_OBJECT_NAME(m_backbufferUAV);

    loadShaders();

    m_cameraConstantBuffer.init(m_device);
    m_shadowCameraConstantBuffer.init(m_device);

    m_cameraConstantBuffer.setName("CameraConstants");
    m_shadowCameraConstantBuffer.setName("ShadowCameraConstants");

    m_postProcessConstants.data.Exposure = 1.0f;
    m_postProcessConstants.data.ScreenSize.x = float(m_width);
    m_postProcessConstants.data.ScreenSize.y = float(m_height);
    m_postProcessConstants.init(m_device);
    m_postProcessConstants.setName("PostProcessConstants");

    m_blur.constants.init(m_device);
    m_blur.constants.setName("GaussianConstants");

    {
        auto dirV = XMVector3Normalize(XMVectorSet(1.0f, -1.0f, 0.0f, 0.0f));

        XMStoreFloat3(&m_psConstants.data.LightDir, dirV);
        m_psConstants.data.NumPointLights = 0;

        m_psConstants.init(m_device);
        m_psConstants.setName("PSConstants");
    }

    {
        m_shadowRT.init(m_device, m_shadowWidth, m_shadowHeight, RT_Depth | RT_DepthSRV, DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);
        m_shadowRT.setName("shadowRT");

        m_shadowSampler = createSamplerState(m_device, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_LESS, nullptr, 0.0f, D3D11_FLOAT32_MAX);
        SET_OBJECT_NAME(m_shadowSampler);

        m_shadowRasterizerState = createRasterizerState(m_device, D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, 0, 0.0f, 0.0f, TRUE,
            FALSE, FALSE, FALSE);
        SET_OBJECT_NAME(m_shadowRasterizerState);
    }

    {
        m_im3dRasterizerState = createRasterizerState(m_device, D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
            0, 0.0f, 0.0f, TRUE, FALSE, FALSE, FALSE);
        SET_OBJECT_NAME(m_im3dRasterizerState);

        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

        m_im3dBlendState = createBlendState(m_device, bd);
        SET_OBJECT_NAME(m_im3dBlendState);

        m_im3dDepthStencilState = createDepthStencilState(m_device,
            TRUE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_GREATER,
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);
        SET_OBJECT_NAME(m_im3dDepthStencilState);

        m_im3dGizmoDepthStencilState = createDepthStencilState(m_device,
            TRUE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_ALWAYS,
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);
        SET_OBJECT_NAME(m_im3dGizmoDepthStencilState);
    }

    {
        m_depthStencilState = createDepthStencilState(m_device,
            TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_GREATER,
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);

        SET_OBJECT_NAME(m_depthStencilState);

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    auto hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    {
        m_framebufferSampler = createSamplerState(m_device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_NEVER, nullptr, 0.0f, D3D11_FLOAT32_MAX);
        SET_OBJECT_NAME(m_framebufferSampler);

        m_mainRT.init(m_device, m_width, m_height, RT_Color | RT_Depth | RT_DepthSRV, hdrFormat,
            DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);

        m_mainRT.setName("mainRT");
    }

    {
        m_batchInstanceBuffer.init(m_device, 128);
        m_batchInstanceBuffer.setName("batchInstanceBuffer");
    }

    {
        auto w = m_width / 2;
        auto h = m_height / 2;

        for (int i = 0; i < NUM_BLUR_PASSES; i++) {
            m_blur.RTs[i].init(m_device, w, h, RT_Color | RT_ColorUAVOnly, hdrFormat);
            m_blur.RTs[i].setName(fmt::format("blurRT_h_{}x{}", w, h));

            m_blur.RTs2[i].init(m_device, w, h, RT_Color | RT_ColorUAVOnly, hdrFormat);
            m_blur.RTs2[i].setName(fmt::format("blurRT_v_{}x{}", w, h));

            w /= 2;
            h /= 2;
        }
    }

    float minLogLuminance = -10.0f;
    float maxLogLuminance = 2.0f;

    m_luminanceHistogramCB.data.inputSize.x = m_mainRT.m_width;
    m_luminanceHistogramCB.data.inputSize.y = m_mainRT.m_height;
    m_luminanceHistogramCB.data.minLogLuminance = minLogLuminance;
    m_luminanceHistogramCB.data.logLuminanceRange = maxLogLuminance - minLogLuminance;
    m_luminanceHistogramCB.data.invLogLuminanceRange = 1.0f / (maxLogLuminance - minLogLuminance);
    m_luminanceHistogramCB.init(m_device);
    m_luminanceHistogramCB.setName("luminance histogram CB");

    {
        std::vector<UINT> tmp(256, 0);
        m_luminanceHistogram.init(m_device, tmp);
        m_luminanceHistogram.setName("luminance histogram");
    }

    m_luminanceHistogramSRV = createShaderResourceView(m_device, m_luminanceHistogram.getBuffer(),
        m_luminanceHistogram.getBuffer(), DXGI_FORMAT_R32_UINT, 0, m_luminanceHistogram.getCapacity());

    m_luminanceHistogramUAV = createUnorderedAccessView(m_device, m_luminanceHistogram.getBuffer(),
        m_luminanceHistogram.getBuffer(), DXGI_FORMAT_R32_TYPELESS, 0, m_luminanceHistogram.getCapacity(),
        D3D11_BUFFER_UAV_FLAG_RAW);

    m_averageLuminance.init(m_device, 1, 1, RT_Color | RT_ColorUAVOnly, DXGI_FORMAT_R32_FLOAT);
    m_averageLuminance.setName("average luminance");
}

Renderable* Renderer::createRenderable(std::string_view name, ArrayView<Vertex> vertices, ArrayView<u16> indices)
{
    auto renderable = std::make_unique<Renderable>();

    const void* p = renderable.get();
    
    renderable->m_vertexBuffer.init(m_device, vertices);
    renderable->m_indexBuffer.init(m_device, indices);
    renderable->m_constantBuffer.init(m_device);

    renderable->m_vertexBuffer.setName(fmt::format("{}_vb", name));
    renderable->m_indexBuffer.setName(fmt::format("{}_ib", name));
    renderable->m_constantBuffer.setName(fmt::format("{}_cb", name));
    renderable->m_name = name;

    return m_renderables.emplace_back(std::move(renderable)).get();
}

void Renderer::setDirectionalLight(const XMFLOAT3& pos, const XMFLOAT3& color, float intensity)
{
    EVENT_SCOPE_FUNC();

    auto dir = XMVector3Normalize(XMLoadFloat3(&pos));
    XMStoreFloat3(&m_psConstants.data.LightDir, dir);
    m_psConstants.data.DirectionalColor = color;
    m_psConstants.data.DepthBias = 0.005f;
    m_psConstants.data.LightIntensity = intensity;

    m_psConstants.update(m_context);
}

void Renderer::setPointLights(ArrayView<PointLight> lights)
{
    EVENT_SCOPE_FUNC();

    if (lights.size > m_pointLights.getCapacity()) {
        m_pointLights.init(m_device, lights.size);
        m_pointLightBufferSRV = createShaderResourceView(m_device, m_pointLights.getBuffer(),
            m_pointLights.getBuffer(), DXGI_FORMAT_UNKNOWN, 0, m_pointLights.getCapacity());
    }

    if (lights.size > 0) {
        m_pointLights.update(m_context, lights);
    }

    m_psConstants.data.NumPointLights = lights.size;
    m_psConstants.update(m_context);
}

void Renderer::draw(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    EVENT_SCOPE("Object {}", renderable->m_name);

    {
        m_cameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    {
        auto wm = transform.getMatrix();
        renderable->m_constantBuffer.data.World = XMMatrixTranspose(wm);
        renderable->m_constantBuffer.data.WorldInvTranspose = XMMatrixInverse(nullptr, wm);
        renderable->m_constantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.getBuffer(), renderable->m_constantBuffer.getBuffer(),
        m_shadowCameraConstantBuffer.getBuffer() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.getBuffer(), m_psConstants.getBuffer() };
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

void Renderer::draw(const RenderBatch& batch, const Camera& camera)
{
    EVENT_SCOPE("Batch {} x{}", batch.renderable->m_name, batch.instances.size());

    m_batchInstanceBuffer.update(m_context, batch.instances);

    {
        m_cameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.getBuffer(),
        m_shadowCameraConstantBuffer.getBuffer() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.getBuffer(), m_psConstants.getBuffer() };
    std::array psResources{ m_pointLightBufferSRV.Get(), m_shadowRT.m_depthSRV.Get() };
    std::array psSamplers{ m_shadowSampler.Get() };

    std::array vertexBuffers{ batch.renderable->m_vertexBuffer.getBuffer(), m_batchInstanceBuffer.getBuffer() };
    std::array strides{ UINT(sizeof(Vertex)), UINT(sizeof(RenderableConstants)) };
    std::array offsets{ UINT(0), UINT(0) };

    m_context->IASetVertexBuffers(0, UINT(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(batch.renderable->m_indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_batchInputLayout.Get());

    m_context->VSSetShader(m_batchVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, UINT(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->PSSetShader(m_ps.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(0, UINT(psConstantBuffers.size()), psConstantBuffers.data());
    m_context->PSSetShaderResources(0, UINT(psResources.size()), psResources.data());
    m_context->PSSetSamplers(0, UINT(psSamplers.size()), psSamplers.data());

    m_context->DrawIndexedInstanced(batch.renderable->m_indexBuffer.getSize(), UINT(batch.instances.size()), 0, 0, 0);
}

void Renderer::drawIm3d(const Camera& camera, ArrayView<Im3d::DrawList> drawLists)
{
    EVENT_SCOPE_FUNC();

    {
        m_cameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.getBuffer(), };

    m_context->RSSetState(m_im3dRasterizerState.Get());

    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_mainRT.m_dsv.Get());
    m_context->OMSetBlendState(m_im3dBlendState.Get(), nullptr, 0xffffffff);

    m_context->IASetInputLayout(m_im3dLayout.Get());

    m_context->VSSetConstantBuffers(0, static_cast<UINT>(vsConstantBuffers.size()), vsConstantBuffers.data());

    const auto gizmoLayerId = Im3d::MakeId("currentEntity");
    for (const auto& drawList : drawLists) {
        if (drawList.m_layerId == gizmoLayerId) {
            // Disable depth testing for the entity gizmo so it won't be hidden by geometry
            m_context->OMSetDepthStencilState(m_im3dGizmoDepthStencilState.Get(), 0);
        } else {
            m_context->OMSetDepthStencilState(m_im3dDepthStencilState.Get(), 0);
        }

        if (drawList.m_primType == Im3d::DrawPrimitive_Lines) {
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            m_context->VSSetShader(m_im3dLineVS.Get(), nullptr, 0);
            m_context->GSSetShader(m_im3dLineGS.Get(), nullptr, 0);
            m_context->PSSetShader(m_im3dLinePS.Get(), nullptr, 0);
        } else if (drawList.m_primType == Im3d::DrawPrimitive_Points) {
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
            m_context->VSSetShader(m_im3dLineVS.Get(), nullptr, 0);
            m_context->GSSetShader(nullptr, nullptr, 0);
            m_context->PSSetShader(m_im3dLinePS.Get(), nullptr, 0);
        } else if (drawList.m_primType == Im3d::DrawPrimitive_Triangles) {
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->VSSetShader(m_im3dTriangleVS.Get(), nullptr, 0);
            m_context->GSSetShader(nullptr, nullptr, 0);
            m_context->PSSetShader(m_im3dTrianglePS.Get(), nullptr, 0);
        } else {
            assert(false);
        }

        if (drawList.m_vertexCount > m_im3dVertexBuffer.getCapacity()) {
            m_im3dVertexBuffer.init(m_device, ArrayView(drawList.m_vertexData, drawList.m_vertexCount));
        } else {
            m_im3dVertexBuffer.update(m_context, ArrayView(drawList.m_vertexData, drawList.m_vertexCount));
        }

        std::array vertexBuffers{ m_im3dVertexBuffer.getBuffer(), };
        std::array strides{ static_cast<UINT>(sizeof(Im3d::VertexData)) };
        std::array offsets{ UINT(0) };

        m_context->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
        m_context->Draw(drawList.m_vertexCount, 0);
    }

    m_context->GSSetShader(nullptr, nullptr, 0);
    m_context->RSSetState(nullptr);
    m_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
}

void Renderer::clear(float r, float g, float b)
{
    EVENT_SCOPE_FUNC();

    float color[4] = { r, g, b, 1.0f };
    m_context->ClearRenderTargetView(m_mainRT.m_framebufferRTV.Get(), color);
    m_context->ClearDepthStencilView(m_mainRT.m_dsv.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
}

void Renderer::beginFrame()
{
    m_annotation->BeginEvent(L"Main pass");

    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_mainRT.m_framebufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_mainRT.m_dsv.Get());
}

void Renderer::endFrame()
{
    m_annotation->EndEvent();
    m_swapChain->Present(1, 0);
}

ID3D11ShaderResourceView* Renderer::computeBloom()
{
    EVENT_SCOPE_FUNC();

    auto sampler = m_framebufferSampler.Get();
    m_context->CSSetSamplers(0, 1, &sampler);

    auto blurPass = [&](const RenderTarget& input, const RenderTarget& output, UINT direction, bool upsample) {
        auto srv = input.m_framebufferSRV.Get();
        auto uav = output.m_framebufferUAV.Get();

        m_context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        m_context->CSSetShaderResources(0, 1, &srv);

        m_blur.constants.data.Direction = direction;

        m_blur.constants.data.InputSize.x = float(input.m_width);
        m_blur.constants.data.InputSize.y = float(input.m_height);

        m_blur.constants.data.OutputSize.x = float(output.m_width);
        m_blur.constants.data.OutputSize.y = float(output.m_height);
        m_blur.constants.data.Upsampling = upsample ? 1 : 0;

        m_blur.constants.update(m_context);

        auto x = UINT(std::ceil(float(output.m_width) / float(TILE_SIZE)));
        auto y = UINT(std::ceil(float(output.m_height) / float(TILE_SIZE)));

        m_context->Dispatch(x, y, 1);
    };

    {
        auto cb = m_blur.constants.getBuffer();
        m_context->CSSetConstantBuffers(0, 1, &cb);
        m_context->CSSetShader(m_blur.gaussianCS.Get(), nullptr, 0);

        auto blurH = [&](auto& input, auto& output) { blurPass(input, output, 0, false); };
        auto blurV = [&](auto& input, auto& output) { blurPass(input, output, 1, false); };
        auto blurUpsampleV = [&](auto& input, auto& output) { blurPass(input, output, 1, true); };

        blurH(      m_mainRT,    m_blur.RTs2[0]);
        blurV(m_blur.RTs2[0],     m_blur.RTs[1]);

        for (int i = 1; i < 4; i++) {
            blurH( m_blur.RTs[i],   m_blur.RTs2[i]);
            blurV(m_blur.RTs2[i],    m_blur.RTs[i+1]);
        }

        for (int i = 4; i > 1; i--) {
            blurH(          m_blur.RTs[i],  m_blur.RTs2[i]);
            blurUpsampleV( m_blur.RTs2[i],   m_blur.RTs[i-1]);
        }

        blurH(  m_blur.RTs[1],  m_blur.RTs2[1]);
        blurV( m_blur.RTs2[1],   m_blur.RTs[0]);
    }

    return m_blur.RTs[0].m_framebufferSRV.Get();
}

void Renderer::postProcess(const PostProcessParams& params)
{
    EVENT_SCOPE_FUNC();

    m_context->OMSetRenderTargets(0, nullptr, nullptr);

    {
        {
            std::array<UINT, 256> tmp;
            std::memset(&tmp, 0, sizeof(tmp));
            m_luminanceHistogram.update(m_context, tmp);

            m_luminanceHistogramCB.data.deltaTime = params.deltaTime;
            m_luminanceHistogramCB.update(m_context);
        }

        std::array resources{ m_mainRT.m_framebufferSRV.Get(), };
        std::array constants{ m_luminanceHistogramCB.getBuffer(), };
        std::array uavs{ m_luminanceHistogramUAV.Get(), m_averageLuminance.m_framebufferUAV.Get(), };

        m_context->CSSetShaderResources(0, UINT(resources.size()), resources.data());
        m_context->CSSetUnorderedAccessViews(0, UINT(uavs.size()), uavs.data(), nullptr);
        m_context->CSSetConstantBuffers(0, UINT(constants.size()), constants.data());
        m_context->CSSetShader(m_luminanceHistogramCS.Get(), nullptr, 0);

        auto x = UINT(std::ceil(float(m_mainRT.m_width) / 16.0f));
        auto y = UINT(std::ceil(float(m_mainRT.m_height) / 16.0f));

        m_context->Dispatch(x, y, 1);

        m_context->CSSetShader(m_luminanceAverageCS.Get(), nullptr, 0);
        m_context->Dispatch(1, 1, 1);
    }

    ID3D11ShaderResourceView* bloomSRV = computeBloom();

    {
        ID3D11UnorderedAccessView* tempUAV[2] = { nullptr, nullptr, };
        ID3D11ShaderResourceView* tempSRV = nullptr;
        m_context->CSSetUnorderedAccessViews(0, 2, tempUAV, nullptr);
        m_context->CSSetShaderResources(0, 1, &tempSRV);
    }

    {
        auto sampler = m_framebufferSampler.Get();
        m_context->CSSetSamplers(0, 1, &sampler);

        m_context->CSSetShader(m_fscs.Get(), nullptr, 0);

        std::array resources{
            m_mainRT.m_framebufferSRV.Get(),
            bloomSRV,
            m_averageLuminance.m_framebufferSRV.Get(),
        };

        std::array constants{
            m_postProcessConstants.getBuffer(),
        };

        std::array uavs{ m_backbufferUAV.Get() };

        m_postProcessConstants.data.Exposure = params.exposure;
        m_postProcessConstants.data.GammaCorrection = params.gammaCorrection ? 1 : 0;
        m_postProcessConstants.update(m_context);

        m_context->CSSetShaderResources(0, UINT(resources.size()), resources.data());
        m_context->CSSetUnorderedAccessViews(0, UINT(uavs.size()), uavs.data(), nullptr);
        m_context->CSSetConstantBuffers(0, UINT(constants.size()), constants.data());

        auto x = UINT(std::ceil(float(m_width) / float(TILE_SIZE)));
        auto y = UINT(std::ceil(float(m_height) / float(TILE_SIZE)));
        m_context->Dispatch(x, y, 1);

        std::memset(resources.data(), 0, sizeof(resources));
        std::memset(uavs.data(), 0, sizeof(uavs));

        m_context->CSSetShaderResources(0, UINT(resources.size()), resources.data());
        m_context->CSSetUnorderedAccessViews(0, UINT(uavs.size()), uavs.data(), nullptr);
    }

    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);
}

void Renderer::beginShadowPass()
{
    m_annotation->BeginEvent(L"Shadow pass");

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

void Renderer::beginShadowPass(const Camera& camera)
{
    m_annotation->BeginEvent(L"Shadow pass");

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

    m_shadowCameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
    m_shadowCameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
    m_shadowCameraConstantBuffer.update(m_context);
}

void Renderer::drawShadow(const RenderBatch& batch)
{
    EVENT_SCOPE("Batch {} x{}", batch.renderable->m_name, batch.instances.size());

    m_batchInstanceBuffer.update(m_context, batch.instances);

    std::array vsConstantBuffers{ m_shadowCameraConstantBuffer.getBuffer() };

    std::array vertexBuffers{ batch.renderable->m_vertexBuffer.getBuffer(), m_batchInstanceBuffer.getBuffer() };
    std::array strides{ UINT(sizeof(Vertex)), UINT(sizeof(RenderableConstants)) };
    std::array offsets{ UINT(0), UINT(0) };

    m_context->IASetVertexBuffers(0, UINT(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(batch.renderable->m_indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_batchInputLayout.Get());

    m_context->VSSetShader(m_shadowBatchVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, UINT(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->DrawIndexedInstanced(batch.renderable->m_indexBuffer.getSize(), UINT(batch.instances.size()), 0, 0, 0);
}

void Renderer::drawShadow(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    EVENT_SCOPE_FUNC();

    {
        m_cameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
        m_cameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
        m_cameraConstantBuffer.update(m_context);

        m_shadowCameraConstantBuffer.data = m_cameraConstantBuffer.data;
        m_shadowCameraConstantBuffer.update(m_context);
    }

    {
        auto wm = transform.getMatrix();

        renderable->m_constantBuffer.data.World = XMMatrixTranspose(wm);
        renderable->m_constantBuffer.data.WorldInvTranspose = XMMatrixInverse(nullptr, wm);
        renderable->m_constantBuffer.update(m_context);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.getBuffer(), renderable->m_constantBuffer.getBuffer() };

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
    
    m_annotation->EndEvent();
}

void Renderer::draw(const RenderBatch& batch)
{
    EVENT_SCOPE("Batch {} x{}", batch.renderable->m_name, batch.instances.size());

    m_batchInstanceBuffer.update(m_context, batch.instances);

    std::array vsConstantBuffers{ m_cameraConstantBuffer.getBuffer(),
        m_shadowCameraConstantBuffer.getBuffer() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.getBuffer(), m_psConstants.getBuffer() };
    std::array psResources{ m_pointLightBufferSRV.Get(), m_shadowRT.m_depthSRV.Get() };
    std::array psSamplers{ m_shadowSampler.Get() };

    std::array vertexBuffers{ batch.renderable->m_vertexBuffer.getBuffer(), m_batchInstanceBuffer.getBuffer() };
    std::array strides{ UINT(sizeof(Vertex)), UINT(sizeof(RenderableConstants)) };
    std::array offsets{ UINT(0), UINT(0) };

    m_context->IASetVertexBuffers(0, UINT(vertexBuffers.size()), vertexBuffers.data(), strides.data(), offsets.data());
    m_context->IASetIndexBuffer(batch.renderable->m_indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_batchInputLayout.Get());

    m_context->VSSetShader(m_batchVS.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, UINT(vsConstantBuffers.size()), vsConstantBuffers.data());

    m_context->PSSetShader(m_ps.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(0, UINT(psConstantBuffers.size()), psConstantBuffers.data());
    m_context->PSSetShaderResources(0, UINT(psResources.size()), psResources.data());
    m_context->PSSetSamplers(0, UINT(psSamplers.size()), psSamplers.data());

    m_context->DrawIndexedInstanced(batch.renderable->m_indexBuffer.getSize(), UINT(batch.instances.size()), 0, 0, 0);
}

void Renderer::beginFrame(const Camera& camera)
{
    m_annotation->BeginEvent(L"Main pass");

    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_mainRT.m_width), static_cast<float>(m_mainRT.m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_mainRT.m_framebufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_mainRT.m_dsv.Get());

    m_cameraConstantBuffer.data.View = camera.getViewMatrix().transposed();
    m_cameraConstantBuffer.data.Projection = camera.getProjectionMatrix().transposed();
    m_cameraConstantBuffer.update(m_context);
}

void Renderer::initImgui()
{
    ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
}

void Renderer::drawImgui()
{
    EVENT_SCOPE_FUNC();

    if (auto drawData = ImGui::GetDrawData()) {
        ImGui_ImplDX11_RenderDrawData(drawData);
    }
}

void Renderer::loadShaders()
{
    const std::filesystem::path shaderDir("../Game");

    m_batchVS = compileVertexShader(m_device, shaderDir / "BatchVertexShader.vs.hlsl", "main",
        [this](ID3DBlob* bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },

                D3D11_INPUT_ELEMENT_DESC{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD_IT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD_IT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD_IT", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "WORLD_IT", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
            };

            for (size_t i = 3; i < layout.size(); i++) {
                layout[i].InstanceDataStepRate = 1;
            }

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode->GetBufferPointer(), bytecode->GetBufferSize(), &m_batchInputLayout);
            setObjectName(m_batchInputLayout, "DefaultBatchInputLayout");
        });

    m_shadowBatchVS = compileVertexShader(m_device, shaderDir / "BatchShadow.vs.hlsl", "main");
    m_shadowVS = compileVertexShader(m_device, shaderDir / "Shadow.vs.hlsl", "main");

    m_ps = compilePixelShader(m_device, shaderDir / "PixelShader.ps.hlsl", "main");
    m_vs = compileVertexShader(m_device, shaderDir / "VertexShader.vs.hlsl", "main",
        [this](ID3DBlob* bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode->GetBufferPointer(), bytecode->GetBufferSize(), &m_inputLayout);
            setObjectName(m_inputLayout, "DefaultInputLayout");
        });

    m_im3dLineGS = compileGeometryShader(m_device, shaderDir / "Im3d.hlsl", "gsLines");
    m_im3dLinePS = compilePixelShader(m_device, shaderDir / "Im3d.hlsl", "psMain");
    m_im3dLineVS = compileVertexShader(m_device, shaderDir / "Im3d.hlsl", "vsMain",
        [this](ID3DBlob* bytecode) {
            std::array layout{
                D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            Hresult hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()),
                bytecode->GetBufferPointer(), bytecode->GetBufferSize(), &m_im3dLayout);
            setObjectName(m_im3dLayout, "Im3dLayout");
        });

    m_im3dTrianglePS = compilePixelShader(m_device, shaderDir / "Im3d.hlsl", "psTriangles");
    m_im3dTriangleVS = compileVertexShader(m_device, shaderDir / "Im3d.hlsl", "vsTriangles");

    m_blur.gaussianCS = compileComputeShader(m_device, shaderDir / "GaussianBlur.cs.hlsl", "main");
    m_fscs = compileComputeShader(m_device, shaderDir / "FullScreenPass.cs.hlsl", "main");

    m_luminanceHistogramCS = compileComputeShader(m_device, shaderDir / "LuminanceHistogram.hlsl", "computeHistogram");
    m_luminanceAverageCS = compileComputeShader(m_device, shaderDir / "LuminanceHistogram.hlsl", "computeAverage");
}

std::unique_ptr<IRenderer> createRenderer(SDL_Window* window)
{
    return std::unique_ptr<IRenderer>(new Renderer(window));
}
