#include "Common.hlsli"

cbuffer PS_Constants : register(b1)
{
    PSConstants pc;
};

StructuredBuffer<PointLight> PointLights : register(t0);

Texture2D ShadowMap : register(t1);
Texture2D Diffuse : register(t2);

SamplerComparisonState ShadowMapSampler : register(s0);
SamplerState LinearSampler : register(s1);

float3 ComputePointLight(PointLight light, float3 position, float3 normal)
{
    float3 l = light.Position.xyz - position;
    float distance = length(l);

    l /= distance;

    float attenuation = 1.0f / (light.Position.w * distance + light.Color.w * distance * distance);
	float ndotl = max(0.0f, dot(normal, l));

    return light.Color.rgb * ndotl * attenuation * light.Intensity;
}

float3 ComputeDirectionalLight(float3 light, float3 normal)
{
    float ndotl = max(0.0f, dot(normal, light.xyz));
    return ndotl * pc.DirectionalColor * pc.LightIntensity;
}

static const float2 PoissonDisk[4] = {
    float2(-0.94201624f, -0.39906216f),
    float2(0.94558609f, -0.76890725f),
    float2(-0.094184101f, -0.92938870f),
    float2(0.34495938f, 0.29387760f)
};

static const float3 g_ambient = float3(0.05f, 0.05f, 0.05f);

float4 main(VS_Output v) : SV_TARGET
{
    float3 n = normalize(v.Normal);

    //float3 total = g_ambient * v.Color.rgb;
    float3 total = g_ambient;
    total += ComputeDirectionalLight(pc.LightDir, n);

    for (uint i = 0; i < pc.NumPointLights; i++) {
        total += ComputePointLight(PointLights[i], v.PositionWS.xyz, n);
    }

    float2 texcoord = v.ShadowPos.xy / v.ShadowPos.w;

    texcoord += 1.0f;
    texcoord *= 0.5;
    texcoord.y = 1.0f - texcoord.y;

    float m = 1.0f;

    for (uint j = 0; j < 4; j++) {
        float depth = ShadowMap.SampleCmpLevelZero(ShadowMapSampler, texcoord + PoissonDisk[j] / 700.0f, v.ShadowPos.z).r;

        if (v.ShadowPos.z < (depth - pc.DepthBias)) {
            m -= 0.2f;
        }
    }

    total *= m;

    return v.Color * float4(total, 1.0f);
    //return Diffuse.Sample(LinearSampler, v.Texcoord * 0.025f) * float4(total, 1.0f);
}