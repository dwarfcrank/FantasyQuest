#include "ShaderCommon.h"

Texture2D<float4> framebuffer : register(t0);
Texture2D<float4> bloomTexture : register(t1);

SamplerState bloomSampler : register(s0);

RWTexture2D<float4> output : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= 1920 || dtid.y >= 1080) {
        return;
    }

    float2 uv = (float2(dtid.xy) + 0.5f) / float2(1920.0f, 1080.0f);
    float4 bl = bloomTexture.SampleLevel(bloomSampler, uv, 0);

    output[dtid.xy] = framebuffer[dtid.xy] + bl;
}