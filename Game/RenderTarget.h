#pragma once

#include "Common.h"

#include <d3d11_1.h>
#include <wrl.h>
#include <string_view>

// TODO: reorder these nicely
enum RenderTargetFlags : u32
{
    RT_Color = (1 << 0),
    RT_Depth = (1 << 1),
    RT_DepthSRV = (1 << 2),
    RT_ColorUAV = (1 << 3),
    RT_ColorUAVOnly = (1 << 4) | RT_ColorUAV,
};

class RenderTarget
{
public:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_framebuffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_framebufferRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_framebufferSRV;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_framebufferUAV;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;

    u32 m_width;
    u32 m_height;

    void init(const Microsoft::WRL::ComPtr<ID3D11Device1>& device, u32 width, u32 height, u32 flags,
        DXGI_FORMAT colorFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthTextureFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT depthSRVFormat = DXGI_FORMAT_UNKNOWN);

    void setName(std::string_view);
};

