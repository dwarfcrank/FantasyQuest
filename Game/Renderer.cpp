#include "Renderer.h"
#include "Common.h"

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

Renderer::Renderer(SDL_Window* window)
{
    UINT devFlags = 0;
    if constexpr (IsDebug) {
        devFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    std::array featureLevels{ D3D_FEATURE_LEVEL_11_1 };

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

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
    sd.Width = static_cast<UINT>(w);
    sd.Height = static_cast<UINT>(h);
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferCount = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(window, &wmi);
    hr = dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), wmi.info.win.window, &sd, nullptr, nullptr, &m_swapChain);

    ComPtr<ID3D11Texture2D> backbuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()));

    hr = m_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &m_backbufferRTV);

    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);

    CD3D11_VIEWPORT vp(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    m_context->RSSetViewports(1, &vp);
}

void Renderer::clear(float r, float g, float b)
{
    float color[4] = { r, g, b, 1.0f };
    m_context->ClearRenderTargetView(m_backbufferRTV.Get(), color);
}

void Renderer::endFrame()
{
    m_swapChain->Present(0, 0);
}
