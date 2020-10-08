#include "ShaderCommon.h"

Texture2D<float4> framebuffer : register(t0);
Texture2D<float4> bloomTexture : register(t1);
Texture2D<float> averageLuminance : register(t2);

SamplerState bloomSampler : register(s0);

RWTexture2D<float4> output : register(u0);

cbuffer cb : register(b0)
{
    PostProcessConstants ppc;
}

static const float gamma = 2.2f;

float3 gammaCorrection(float3 color)
{
    return pow(color, 1.0f / gamma);
}

// copypasta from https://www.shadertoy.com/view/lslGzl
float3 Uncharted2ToneMapping(float3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
    color *= 1.0f / averageLuminance[uint2(0, 0)];
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return color;
}

float4 computeToneMapping(float4 color)
{
    float4 result = 0.0f;

    result.rgb = Uncharted2ToneMapping(color.rgb);

    if (ppc.GammaCorrection != 0) {
		result.rgb = gammaCorrection(result.rgb);
    }

    return result;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float2 xy = float2(dtid.xy);

    if (any(xy >= ppc.ScreenSize)) {
        return;
    }

    float2 uv = xy / ppc.ScreenSize;
    float4 bl = bloomTexture.SampleLevel(bloomSampler, uv, 0);

    output[dtid.xy] = computeToneMapping(lerp(framebuffer[dtid.xy], bl, 0.04f));
}