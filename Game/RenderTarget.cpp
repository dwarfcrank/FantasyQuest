#include "pch.h"

#include "Common.h"
#include "RenderTarget.h"
#include "RendererHelpers.h"

void RenderTarget::init(const Microsoft::WRL::ComPtr<ID3D11Device1>& device,
    u32 width, u32 height, u32 flags, 
    DXGI_FORMAT colorFormat, DXGI_FORMAT depthTextureFormat, DXGI_FORMAT depthFormat, DXGI_FORMAT depthSRVFormat)
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
