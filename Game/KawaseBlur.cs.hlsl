#include "ShaderCommon.h"

cbuffer cb : register(b0)
{
    KawaseConstants kc;
};

Texture2D<float4> framebuffer : register(t0);
SamplerState fbSampler : register(s0);

RWTexture2D<float4> output : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    //float2 uv = (float2(dtid.xy) + 0.5f) / kc.OutputSize;
    float2 uv = float2(dtid.xy) / kc.OutputSize;
    float2 offset = kc.KernelSize / kc.InputSize;

    float4 result = 0.0f;

    result += framebuffer.SampleLevel(fbSampler, uv + float2( offset.x,  offset.y), 0) * 0.25f;
    result += framebuffer.SampleLevel(fbSampler, uv + float2(-offset.x,  offset.y), 0) * 0.25f;
    result += framebuffer.SampleLevel(fbSampler, uv + float2( offset.x, -offset.y), 0) * 0.25f;
    result += framebuffer.SampleLevel(fbSampler, uv + float2(-offset.x, -offset.y), 0) * 0.25f;

    if (kc.Upsampling != 0) {
		output[dtid.xy] += result;
    } else {
		output[dtid.xy] = result;
    }
}