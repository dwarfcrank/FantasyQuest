#pragma once

#include "ArrayView.h"
#include "Hresult.h"

#include <wrl.h>
#include <d3d11_1.h>

template<typename... Args>
auto createTexture2D(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_TEXTURE2D_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Hresult hr = m_device->CreateTexture2D(&desc, nullptr, &texture);
    return texture;
}

template<typename... Args>
auto createBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_BUFFER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Hresult hr = m_device->CreateBuffer(&desc, nullptr, &buffer);
    return buffer;
}

template<typename T, typename... Args>
auto createBufferWithData(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ArrayView<T> contents, Args... args)
{
    CD3D11_BUFFER_DESC desc(contents.byteSize(), args...);
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = contents.data;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Hresult hr = m_device->CreateBuffer(&desc, &sd, &buffer);
    return buffer;
}

template<typename... Args>
auto createRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_RENDER_TARGET_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Hresult hr = m_device->CreateRenderTargetView(resource, &desc, &rtv);
    return rtv;
}

auto createRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource)
{
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Hresult hr = m_device->CreateRenderTargetView(resource, nullptr, &rtv);
    return rtv;
}

template<typename... Args>
auto createShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_SHADER_RESOURCE_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Hresult hr = m_device->CreateShaderResourceView(resource, &desc, &srv);
    return srv;
}

template<typename... Args>
auto createDepthStencilView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_DEPTH_STENCIL_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
    Hresult hr = m_device->CreateDepthStencilView(resource, &desc, &dsv);
    return dsv;
}

template<typename... Args>
auto createDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_DEPTH_STENCIL_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss;
    Hresult hr = m_device->CreateDepthStencilState(&desc, &dss);
    return dss;
}

template<typename... Args>
auto createSamplerState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_SAMPLER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11SamplerState> ss;
    Hresult hr = m_device->CreateSamplerState(&desc, &ss);
    return ss;
}

template<typename... Args>
auto createRasterizerState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_RASTERIZER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rs;
    Hresult hr = m_device->CreateRasterizerState(&desc, &rs);
    return rs;
}
