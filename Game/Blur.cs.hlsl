#include "ShaderCommon.h"

cbuffer cb : register(b0)
{
    BlurConstants bc;
};

Texture2D<float4> framebuffer : register(t0);
SamplerState fbSampler : register(s0);

RWTexture2D<float4> output : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float4 result = (float4)0;

    float offset = bc.KernelSize;
    //float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + float2(0.5f, 0.5f);
    float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + (bc.TexcoordScale - 1.0f) + float2(0.5f, 0.5f);

    result += framebuffer.SampleLevel(fbSampler, ((uv + float2( offset,  offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2( offset, -offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset,  offset)) / bc.ScreenSize), 0);
    result += framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset, -offset)) / bc.ScreenSize), 0);

    result *= 0.25f;

    output[dtid.xy] = result;
}