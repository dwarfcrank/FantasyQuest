#include "pch.h"

#include "Renderer.h"
#include "Common.h"
#include "File.h"
#include "Transform.h"

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
ComPtr<ID3D11Buffer> createBuffer(const ComPtr<ID3D11Device1>& device, UINT flags, const std::vector<T>& contents)
{
    static_assert(std::is_standard_layout_v<T>);
    CD3D11_BUFFER_DESC bd(static_cast<UINT>(sizeof(T) * contents.size()), flags);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = contents.data();

    ComPtr<ID3D11Buffer> buf;
    Hresult hr = device->CreateBuffer(&bd, &sd, &buf);

    return buf;
}

template<typename T>
ComPtr<ID3D11Buffer> createBuffer(const ComPtr<ID3D11Device1>& device, UINT flags, const T& contents)
{
    static_assert(std::is_standard_layout_v<T>);
    CD3D11_BUFFER_DESC bd(static_cast<UINT>(sizeof(T)), flags);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = &contents;

    ComPtr<ID3D11Buffer> buf;
    Hresult hr = device->CreateBuffer(&bd, &sd, &buf);

    return buf;
}

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

    hr = m_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &m_backbufferRTV);

    {
        auto vs = loadFile("../x64/Debug/VertexShader.cso");
        auto ps = loadFile("../x64/Debug/PixelShader.cso");

        hr = m_device->CreateVertexShader(vs.data(), vs.size(), nullptr, &m_vs);
        hr = m_device->CreatePixelShader(ps.data(), ps.size(), nullptr, &m_ps);

        std::array layout{
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = m_device->CreateInputLayout(layout.data(), static_cast<UINT>(layout.size()), vs.data(), vs.size(), &m_inputLayout);
    }

    {
        auto vs = loadFile("../x64/Debug/ShadowVertexShader.cso");
        auto ps = loadFile("../x64/Debug/ShadowPixelShader.cso");

        hr = m_device->CreateVertexShader(vs.data(), vs.size(), nullptr, &m_shadowVS);
        hr = m_device->CreatePixelShader(ps.data(), ps.size(), nullptr, &m_shadowPS);
    }

    m_cameraConstantBuffer = createBuffer(m_device, D3D11_BIND_CONSTANT_BUFFER, CameraConstantBuffer{});
    m_shadowCameraConstantBuffer = createBuffer(m_device, D3D11_BIND_CONSTANT_BUFFER, CameraConstantBuffer{});

    {
        auto dir = XMVector3Normalize(XMVectorSet(1.0f, 1.0f, -1.0f, 0.0f));

        XMStoreFloat3(&m_psConstants.LightPosition, dir);
        m_psConstants.NumPointLights = 0;

        m_psConstantBuffer = createBuffer(m_device, D3D11_BIND_CONSTANT_BUFFER, m_psConstants);
    }

    {
        CD3D11_TEXTURE2D_DESC dsd(DXGI_FORMAT_D24_UNORM_S8_UINT, m_width, m_height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
        hr = m_device->CreateTexture2D(&dsd, nullptr, &m_depthStencilTexture);

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvd(D3D11_DSV_DIMENSION_TEXTURE2D, dsd.Format);
        hr = m_device->CreateDepthStencilView(m_depthStencilTexture.Get(), &dsvd, &m_depthStencilView);
    }

    {
        CD3D11_TEXTURE2D_DESC dsd(DXGI_FORMAT_R32_TYPELESS, m_shadowWidth, m_shadowHeight, 1, 1,
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
        hr = m_device->CreateTexture2D(&dsd, nullptr, &m_shadowTexture);

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvd(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
        hr = m_device->CreateDepthStencilView(m_shadowTexture.Get(), &dsvd, &m_shadowDSV);

        CD3D11_SHADER_RESOURCE_VIEW_DESC dsrvd(m_shadowTexture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D,
            DXGI_FORMAT_R32_FLOAT);
        hr = m_device->CreateShaderResourceView(m_shadowTexture.Get(), &dsrvd, &m_shadowSRV);

        CD3D11_SAMPLER_DESC tsd(D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
            0.0f, 0, D3D11_COMPARISON_NEVER, nullptr, 0.0f, D3D11_FLOAT32_MAX);
        hr = m_device->CreateSamplerState(&tsd, &m_shadowSampler);

        CD3D11_RASTERIZER_DESC rd(D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, 0, 0.0f, 0.0f, TRUE,
            FALSE, FALSE, FALSE);
        hr = m_device->CreateRasterizerState(&rd, &m_shadowRasterizerState);
    }
}

Renderable* Renderer::createRenderable(const std::vector<Vertex>& vertices, const std::vector<u16>& indices)
{
    std::unique_ptr<Renderable> renderable(new Renderable());

    renderable->m_vertexBuffer = createBuffer(m_device.Get(), D3D11_BIND_VERTEX_BUFFER, vertices);
    renderable->m_vertexCount = static_cast<UINT>(vertices.size());

    renderable->m_indexBuffer = createBuffer(m_device.Get(), D3D11_BIND_INDEX_BUFFER, indices);
    renderable->m_indexCount = static_cast<UINT>(indices.size());

    RenderableConstantBuffer cb{
        .WorldMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f),
    };

    renderable->m_constantBuffer = createBuffer(m_device.Get(), D3D11_BIND_CONSTANT_BUFFER, cb);

    return m_renderables.emplace_back(std::move(renderable)).get();
}

void Renderer::setDirectionalLight(const XMFLOAT3& pos)
{
    auto dir = XMVector3Normalize(XMLoadFloat3(&pos));
    XMStoreFloat3(&m_psConstants.LightPosition, dir);

    m_context->UpdateSubresource(m_psConstantBuffer.Get(), 0, nullptr, &m_psConstants, 0, 0);
}

void Renderer::setPointLights(const std::vector<PointLight>& lights)
{
    m_psConstants.NumPointLights = static_cast<UINT>(lights.size());

    if (lights.size() > m_pointLightCapacity) {
        CD3D11_BUFFER_DESC bd(static_cast<UINT>(sizeof(PointLight) * lights.size()), D3D11_BIND_SHADER_RESOURCE);
        bd.StructureByteStride = sizeof(PointLight);
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        Hresult hr = m_device->CreateBuffer(&bd, nullptr, &m_pointLightBuffer);

        m_pointLightCapacity = static_cast<UINT>(lights.size());
        CD3D11_SHADER_RESOURCE_VIEW_DESC desc(m_pointLightBuffer.Get(), DXGI_FORMAT_UNKNOWN, 0, m_pointLightCapacity);
        hr = m_device->CreateShaderResourceView(m_pointLightBuffer.Get(), &desc, &m_pointLightBufferSRV);
    }

    CD3D11_BOX box(0, 0, 0, sizeof(PointLight) * m_psConstants.NumPointLights, 1, 1);

    m_context->UpdateSubresource(m_pointLightBuffer.Get(), 0, &box, lights.data(),
        static_cast<UINT>(sizeof(PointLight) * m_psConstants.NumPointLights), 0);

    m_context->UpdateSubresource(m_psConstantBuffer.Get(), 0, nullptr, &m_psConstants, 0, 0);
}

void Renderer::draw(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    {
        CameraConstantBuffer cb{
            .ViewMatrix = XMMatrixTranspose(camera.getViewMatrix()),
            .ProjectionMatrix = XMMatrixTranspose(camera.getProjectionMatrix()),
        };

        m_context->UpdateSubresource(m_cameraConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    {
        auto wm = transform.getMatrix();
        RenderableConstantBuffer cb{
            .WorldMatrix = XMMatrixTranspose(wm),
            .WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm),
        };

        m_context->UpdateSubresource(renderable->m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.Get(), renderable->m_constantBuffer.Get(),
        m_shadowCameraConstantBuffer.Get() };
    std::array psConstantBuffers{ m_cameraConstantBuffer.Get(), m_psConstantBuffer.Get() };
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

void Renderer::clear(float r, float g, float b)
{
    float color[4] = { r, g, b, 1.0f };
    m_context->ClearRenderTargetView(m_backbufferRTV.Get(), color);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::beginFrame()
{
    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_context->RSSetViewports(1, &vp);

    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_depthStencilView.Get());
}

void Renderer::endFrame()
{
    m_swapChain->Present(0, 0);
}

void Renderer::beginShadowPass()
{
    // Make sure the shadow stuff isn't bound
    std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> srvs;
    std::memset(srvs.data(), 0, sizeof(srvs));
    m_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs.data());

    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(m_shadowWidth), static_cast<float>(m_shadowHeight));
    m_context->RSSetViewports(1, &vp);

    m_context->ClearDepthStencilView(m_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_context->OMSetRenderTargets(0, nullptr, m_shadowDSV.Get());

    m_context->PSSetShader(nullptr, nullptr, 0);
    m_context->RSSetState(m_shadowRasterizerState.Get());
}

void Renderer::drawShadow(Renderable* renderable, const Camera& camera, const Transform& transform)
{
    {
        CameraConstantBuffer cb{
            .ViewMatrix = XMMatrixTranspose(camera.getViewMatrix()),
            .ProjectionMatrix = XMMatrixTranspose(camera.getProjectionMatrix()),
        };

        m_context->UpdateSubresource(m_cameraConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
        m_context->UpdateSubresource(m_shadowCameraConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    {
        auto wm = transform.getMatrix();
        RenderableConstantBuffer cb{
            .WorldMatrix = XMMatrixTranspose(wm),
            .WorldInvTransposeMatrix = XMMatrixInverse(nullptr, wm),
        };

        m_context->UpdateSubresource(renderable->m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    std::array vsConstantBuffers{ m_cameraConstantBuffer.Get(), renderable->m_constantBuffer.Get() };

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

XMMATRIX Camera::getViewMatrix() const
{
    return XMMatrixLookToLH(m_position, m_direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

XMMATRIX Camera::getProjectionMatrix() const
{
    return m_projectionMatrix;
}

void Camera::move(float xOff, float yOff, float zOff)
{
    m_position += XMVectorSet(xOff, yOff, zOff, 0.0f);
}

void Camera::setPosition(float x, float y, float z)
{
    m_position = XMVectorSet(x, y, z, 0.0f);
}
