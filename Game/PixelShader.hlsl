#include "Common.hlsli"

cbuffer PS_Constants : register(b1)
{
    float3 LightDir;
    uint NumPointLights;
};

struct PointLight
{
    float4 Position; // .w = linear attenuation
    float4 Color; // .w = quadratic attenuation
};

StructuredBuffer<PointLight> PointLights : register(t0);

Texture2D ShadowMap : register(t1);
SamplerState ShadowMapSampler : register(s0);

float3 ComputePointLight(PointLight light, float3 position, float3 normal)
{
    float3 l = light.Position.xyz - position;
    float distance = length(l);

    l /= distance;

    float attenuation = 1.0f / (light.Position.w * distance + light.Color.w * distance * distance);
	float ndotl = max(0.0f, dot(normal, l));

    return light.Color.rgb * ndotl * attenuation;
}

float3 ComputeDirectionalLight(float3 light, float3 normal)
{
    float ndotl = max(0.0f, dot(normal, light.xyz));
    // TODO: implement color for directional light
	return ndotl * float3(1.0f, 1.0f, 1.0f);
}

float4 main(VS_Output v) : SV_TARGET
{
    float3 total = ComputeDirectionalLight(LightDir, v.Normal);

    for (uint i = 0; i < NumPointLights; i++) {
        total += ComputePointLight(PointLights[i], v.PositionWS.xyz, v.Normal);
    }

    float2 texcoord = v.ShadowPos.xy;

    texcoord += 1.0f;
    texcoord *= 0.5;
    texcoord.y = 1.0f - texcoord.y;

    float depth = ShadowMap.Sample(ShadowMapSampler, texcoord).r;
    if (v.ShadowPos.z < (depth - 0.005f)) {
        total *= 0.5f;
    }

    return v.Color * float4(total, 1.0f);
}