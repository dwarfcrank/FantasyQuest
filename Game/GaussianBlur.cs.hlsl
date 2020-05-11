#include "ShaderCommon.h"

cbuffer cb : register(b0)
{
    BlurConstants bc;
};

Texture2D<float4> framebuffer : register(t0);
SamplerState fbSampler : register(s0);

RWTexture2D<float4> output : register(u0);

static const float g_kernel[5] = {
    0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f
};

static const float4 g_LumaVector = float4(0.299f, 0.587f, 0.114f, 0.0f);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float4 result = (float4)0;
    float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + (bc.TexcoordScale - 1.0f) + float2(0.5f, 0.5f);
    //float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + float2(0.5f, 0.5f);

    if (bc.BlurPass == 0) {
        for (int i = -2; i <= 2; i++) {
            result += framebuffer.SampleLevel(fbSampler, ((uv + float2(i, 0)) / bc.ScreenSize), 0) * g_kernel[i + 2];
        }
    } else if (bc.BlurPass == 1) {
        for (int i = -2; i <= 2; i++) {
            result += framebuffer.SampleLevel(fbSampler, ((uv + float2(0, i)) / bc.ScreenSize), 0) * g_kernel[i + 2];
        }
    } else if (bc.BlurPass == 2) {
        for (int i = -2; i <= 2; i++) {
            float4 texel = framebuffer.SampleLevel(fbSampler, ((uv + float2(i, 0)) / bc.ScreenSize), 0);
            if (dot(texel, g_LumaVector) > 0.5f) {
                result += texel * g_kernel[i + 2];
            }
        }
    }

    /*
    float offset = bc.KernelSize;
    //float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + float2(0.5f, 0.5f);

    result += framebuffer.SampleLevel(fbSampler, ((uv + float2( offset,  offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2( offset, -offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset,  offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset, -offset)) / bc.ScreenSize), 0);

    result *= 0.25f;
    */

    output[dtid.xy] = result;
}