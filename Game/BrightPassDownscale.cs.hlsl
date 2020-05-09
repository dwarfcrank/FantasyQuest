#include "ShaderCommon.h"

cbuffer cb : register(b0)
{
    BlurConstants bc;
};

Texture2D<float4> framebuffer : register(t0);
SamplerState fbSampler : register(s0);

RWTexture2D<float4> output : register(u0);

static const float4 g_LumaVector = float4(0.299f, 0.587f, 0.114f, 0.0f);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float4 result = (float4)0;

    float offset = bc.KernelSize;
    //float2 uv = (float2(dtid.xy) * 2.0f) + 1.5f;// +float2(0.5f, 0.5f);
    float2 uv = (float2(dtid.xy) * bc.TexcoordScale) + (bc.TexcoordScale - 1.0f) + float2(0.5f, 0.5f);

    float4 colors[4];

    colors[0] = framebuffer.SampleLevel(fbSampler, ((uv + float2( offset,  offset)) / bc.ScreenSize), 0);
    colors[1] = framebuffer.SampleLevel(fbSampler, ((uv + float2( offset, -offset)) / bc.ScreenSize), 0);
    colors[2] = framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset,  offset)) / bc.ScreenSize), 0);
    colors[3] = framebuffer.SampleLevel(fbSampler, ((uv + float2(-offset, -offset)) / bc.ScreenSize), 0);

    int numBright = 0;

    /*
    for (int i = 0; i < 4; i++) {
        if (dot(colors[i], g_LumaVector) >= 0.5f) {
            numBright++;
            result += colors[i];
        }
    }

    if (numBright > 0) {
        result /= (float)numBright;
    }
    */

    for (int i = 0; i < 4; i++) {
        if (dot(colors[i], g_LumaVector) >= 0.5f) {
            result += colors[i] * 0.25f;
        }
    }

    output[dtid.xy] = result;
}
