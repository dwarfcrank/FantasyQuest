#include "Common.hlsli"

static const float3 g_LightDir = float3(0.5773502691896258, 0.5773502691896258, -0.5773502691896258);

float4 main(VS_Output v) : SV_TARGET
{
	float ndotl = max(0.0f, dot(v.Normal, g_LightDir));
    return float4(v.Color.xyz * ndotl, 1.0f);
}