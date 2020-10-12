#include "../pch.h"

#include "RenderContext.h"
#include "../RenderTarget.h"

#include <d3d11_4.h>
#include <cassert>

void RenderContext::clearRenderTarget(RenderTarget* rt, const DirectX::XMFLOAT4& clearColor, float depth)
{
    if (auto rtv = rt->m_framebufferRTV.Get()) {
        m_context->ClearRenderTargetView(rtv, &clearColor.x);
    }

    if (auto dsv = rt->m_dsv.Get()) {
        m_context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, depth, 0);
    }
}

void RenderContext::bindRenderTarget(RenderTarget* rt)
{
    auto rtv = rt->m_framebufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, rt->m_dsv.Get());

    CD3D11_VIEWPORT vp(0.0f, 0.0f, float(rt->m_width), float(rt->m_height), 0.0f, 1.0f);
    m_context->RSSetViewports(1, &vp);
}

void RenderContext::bindBackbuffer()
{
    auto rtv = m_backbufferRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);

    CD3D11_VIEWPORT vp(0.0f, 0.0f, float(m_backbufferSize.x), float(m_backbufferSize.y), 0.0f, 1.0f);
    m_context->RSSetViewports(1, &vp);
}

void RenderContext::draw(const DrawParams& p)
{
    assert(p.vs.shader);
    assert(p.indexBuffer);

    m_context->VSSetShader(p.vs.shader, nullptr, 0);
    if (!p.vs.constants.empty()) {
        m_context->VSSetConstantBuffers(0, u32(p.vs.constants.size()), p.vs.constants.data());
    }

    if (!p.vs.resources.empty()) {
        m_context->VSSetShaderResources(0, u32(p.vs.resources.size()), p.vs.resources.data());
    }

    if (!p.vs.samplers.empty()) {
        m_context->VSSetSamplers(0, u32(p.vs.samplers.size()), p.vs.samplers.data());
    }

    m_context->IASetInputLayout(p.vs.inputLayout);
    m_context->IASetPrimitiveTopology(p.primitiveTopology);

    if (!p.vertexBuffers.buffers.empty()) {
        const auto& v = p.vertexBuffers.buffers;
        const auto& o = p.vertexBuffers.offsets;
        const auto& s = p.vertexBuffers.strides;

        assert(v.size() == s.size());
        assert(v.size() == o.size());

        m_context->IASetVertexBuffers(0, u32(v.size()), v.data(), s.data(), o.data());
    }

    m_context->PSSetShader(p.ps.shader, nullptr, 0);
    if (p.ps.shader) {
        if (!p.ps.constants.empty()) {
            m_context->PSSetConstantBuffers(0, u32(p.ps.constants.size()), p.ps.constants.data());
        }

        if (!p.ps.resources.empty()) {
            m_context->PSSetShaderResources(0, u32(p.ps.resources.size()), p.ps.resources.data());
        }

        if (!p.ps.samplers.empty()) {
            m_context->PSSetSamplers(0, u32(p.ps.samplers.size()), p.ps.samplers.data());
        }
    }

    m_context->GSSetShader(p.gs.shader, nullptr, 0);
    if (p.gs.shader) {
        if (!p.gs.constants.empty()) {
            m_context->GSSetConstantBuffers(0, u32(p.gs.constants.size()), p.gs.constants.data());
        }

        if (!p.gs.resources.empty()) {
            m_context->GSSetShaderResources(0, u32(p.gs.resources.size()), p.gs.resources.data());
        }

        if (!p.gs.samplers.empty()) {
            m_context->GSSetSamplers(0, u32(p.gs.samplers.size()), p.gs.samplers.data());
        }
    }

    m_context->IASetIndexBuffer(p.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->DrawIndexedInstanced(p.numIndices, p.numInstances, p.baseIndex, 0, 0);
}

void RenderContext::compute(const ComputeParams& p)
{
    assert(p.shader);
    assert(p.threads.x > 0 && p.threads.y > 0 && p.threads.z > 0);

    m_context->CSSetShader(p.shader, nullptr, 0);

    if (!p.constants.empty()) {
        m_context->CSSetConstantBuffers(0, u32(p.constants.size()), p.constants.data());
    }

    if (!p.resources.empty()) {
        m_context->CSSetShaderResources(0, u32(p.resources.size()), p.resources.data());
    }

    if (!p.samplers.empty()) {
        m_context->CSSetSamplers(0, u32(p.samplers.size()), p.samplers.data());
    }

    if (!p.uavs.empty()) {
        m_context->CSSetUnorderedAccessViews(0, u32(p.uavs.size()), p.uavs.data(), nullptr);
    }

    m_context->Dispatch(p.threads.x, p.threads.y, p.threads.z);
}
