#include "pch.h"

#include "Common.h"
#include "RenderTarget.h"
#include "RendererHelpers.h"

#include <fmt/format.h>

void RenderTarget::init(const Microsoft::WRL::ComPtr<ID3D11Device1>& device,
    u32 width, u32 height, u32 flags, 
    DXGI_FORMAT colorFormat, DXGI_FORMAT depthTextureFormat, DXGI_FORMAT depthFormat, DXGI_FORMAT depthSRVFormat)
{
    m_width = width;
    m_height = height;

    if (flags & RT_Color) {
        UINT colorFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (flags & RT_ColorUAV) {
            colorFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (flags & RT_ColorUAVOnly) {
            colorFlags &= ~D3D11_BIND_RENDER_TARGET;
        }

        m_framebuffer = createTexture2D(device, colorFormat, m_width, m_height, 1, 1,
            colorFlags);

        if (colorFlags & D3D11_BIND_RENDER_TARGET) {
            m_framebufferRTV = createRenderTargetView(device, m_framebuffer.Get());
        }

        if (flags & RT_ColorUAV) {
            m_framebufferUAV = createUnorderedAccessView(device, m_framebuffer.Get());
        }

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

void RenderTarget::setName(std::string_view name)
{
    if (m_framebuffer) {
        setObjectName(m_framebuffer, fmt::format("{}_fb_tex", name));

        if (m_framebufferRTV) {
            setObjectName(m_framebufferRTV, fmt::format("{}_fb_rtv", name));
        }

        if (m_framebufferUAV) {
            setObjectName(m_framebufferUAV, fmt::format("{}_fb_uav", name));
        }

        setObjectName(m_framebufferSRV, fmt::format("{}_fb_srv", name));
    }

    if (m_depthTexture) {
        setObjectName(m_depthTexture, fmt::format("{}_d_tex", name));
        setObjectName(m_dsv, fmt::format("{}_d_dsv", name));

        if (m_depthSRV) {
            setObjectName(m_depthSRV, fmt::format("{}_d_srv", name));
        }
    }
}
