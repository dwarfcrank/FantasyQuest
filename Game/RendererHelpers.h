#pragma once

#include "ArrayView.h"
#include "Hresult.h"

#include <wrl.h>
#include <d3d11_1.h>
#include <string_view>

template<typename... Args>
inline auto createTexture2D(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_TEXTURE2D_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Hresult hr = m_device->CreateTexture2D(&desc, nullptr, &texture);
    return texture;
}

template<typename... Args>
inline auto createBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_BUFFER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Hresult hr = m_device->CreateBuffer(&desc, nullptr, &buffer);
    return buffer;
}

template<typename T, typename... Args>
inline auto createBufferWithData(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ArrayView<T> contents, Args... args)
{
    CD3D11_BUFFER_DESC desc(contents.byteSize(), args...);
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = contents.data;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Hresult hr = m_device->CreateBuffer(&desc, &sd, &buffer);
    return buffer;
}

template<typename... Args>
inline auto createRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_RENDER_TARGET_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Hresult hr = m_device->CreateRenderTargetView(resource, &desc, &rtv);
    return rtv;
}

inline auto createRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource)
{
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Hresult hr = m_device->CreateRenderTargetView(resource, nullptr, &rtv);
    return rtv;
}

template<typename... Args>
inline auto createShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_SHADER_RESOURCE_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Hresult hr = m_device->CreateShaderResourceView(resource, &desc, &srv);
    return srv;
}

template<typename... Args>
inline auto createDepthStencilView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    CD3D11_DEPTH_STENCIL_VIEW_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
    Hresult hr = m_device->CreateDepthStencilView(resource, &desc, &dsv);
    return dsv;
}

template<typename... Args>
inline auto createDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_DEPTH_STENCIL_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss;
    Hresult hr = m_device->CreateDepthStencilState(&desc, &dss);
    return dss;
}

template<typename... Args>
inline auto createSamplerState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_SAMPLER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11SamplerState> ss;
    Hresult hr = m_device->CreateSamplerState(&desc, &ss);
    return ss;
}

template<typename... Args>
inline auto createRasterizerState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_RASTERIZER_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rs;
    Hresult hr = m_device->CreateRasterizerState(&desc, &rs);
    return rs;
}

template<typename... Args>
inline auto createBlendState(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, Args... args)
{
    CD3D11_BLEND_DESC desc(args...);
    Microsoft::WRL::ComPtr<ID3D11BlendState> bs;
    Hresult hr = m_device->CreateBlendState(&desc, &bs);
    return bs;
}

template<typename... Args>
inline auto createUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11Device>& m_device, ID3D11Resource* resource, Args... args)
{
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
    Hresult hr;

    if constexpr (sizeof...(args) > 0) {
        CD3D11_UNORDERED_ACCESS_VIEW_DESC desc(args...);
        hr = m_device->CreateUnorderedAccessView(resource, &desc, &uav);
    } else {
        hr = m_device->CreateUnorderedAccessView(resource, nullptr, &uav);
    }

    return uav;
}

inline void setObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& resource, std::string_view name)
{
    if constexpr (IsDebug) {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.length()), name.data());
    }
}

#define SET_OBJECT_NAME(obj) setObjectName(obj, #obj)

#ifdef _DEBUG

class Annotation
{
public:
    template<typename... Args>
    Annotation(const Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>& annotation, const char* format, Args&&... args) :
        m_annotation(annotation)
    {
        beginEvent(fmt::format(format, std::forward<Args>(args)...));
    }

    Annotation(const Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>& annotation, std::string_view message) :
        m_annotation(annotation)
    {
        beginEvent(message);
    }

    ~Annotation()
    {
        m_annotation->EndEvent();
    }

private:
    void beginEvent(std::string_view message)
    {
        std::array<WCHAR, 128> buf;
        std::memset(buf.data(), 0, sizeof(buf));

        if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, message.data(), int(message.size()),
            buf.data(), int(buf.size())) == 0) {
            throw std::runtime_error("String conversion failed");
        }

        m_annotation->BeginEvent(buf.data());
    }

    const Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>& m_annotation;
};

#define TOKENPASTE2(a, b) a##b
#define TOKENPASTE(a, b) TOKENPASTE2(a, b)
#define EVENT_SCOPE(...) Annotation TOKENPASTE(_annotation_, __COUNTER__)(m_annotation, __VA_ARGS__)
#define EVENT_SCOPE_FUNC() EVENT_SCOPE(__func__)

#else

#define EVENT_SCOPE(...)
#define EVENT_SCOPE_FUNC()

#endif
