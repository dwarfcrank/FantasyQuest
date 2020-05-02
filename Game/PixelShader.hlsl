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
    //float Radius;
};

//static const float3 g_LightDir = float3(0.5773502691896258, 0.5773502691896258, -0.5773502691896258);

StructuredBuffer<PointLight> PointLights : register(t0);

float3 ComputeLight(PointLight light, float3 position, float3 normal)
{
    float3 l = light.Position.xyz - position;
    float distance = length(l);

    l /= distance;

    float attenuation = 1.0f / (light.Position.w * distance + light.Color.w * distance * distance);

	float ndotl = max(0.0f, dot(normal, l));

    return light.Color.rgb * ndotl * attenuation;
}

float4 main(VS_Output v) : SV_TARGET
{
    //float ndotl = max(0.0f, dot(v.Normal, g_LightDir));
    float4 color = v.Color;

	float ndotl = max(0.0f, dot(v.Normal, LightDir.xyz));
    color.rgb = v.Color.rgb * ndotl;

    for (uint i = 0; i < NumPointLights; i++) {
        color.rgb += ComputeLight(PointLights[i], v.PositionWS.xyz, v.Normal);
    }

    return color;
}