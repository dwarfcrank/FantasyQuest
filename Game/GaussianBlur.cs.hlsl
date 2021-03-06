#include "ShaderCommon.h"

cbuffer cb : register(b0)
{
    GaussianConstants gc;
};

Texture2D<float4> framebuffer : register(t0);
SamplerState fbSampler : register(s0);

RWTexture2D<float4> output : register(u0);

static const float g_kernel[5] = {
	0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f
};

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float2 uv = (float2(dtid.xy) + 0.5f) / gc.OutputSize;

    float2 offset = 0.0f;

    if (gc.Direction == 0) {
        offset.x = 1.0f;
    } else if (gc.Direction == 1) {
        offset.y = 1.0f;
    }

    offset /= gc.OutputSize;

    float4 result = framebuffer.SampleLevel(fbSampler, uv, 0) * g_kernel[0];

    for (int i = 1; i < 5; i++) {
        float2 off = float(i) * offset;
        result += framebuffer.SampleLevel(fbSampler, uv + off, 0) * g_kernel[i];
        result += framebuffer.SampleLevel(fbSampler, uv - off, 0) * g_kernel[i];
    }

    if (gc.Upsampling == 0) {
		output[dtid.xy] = result;
    } else {
		output[dtid.xy] += result;
    }
}