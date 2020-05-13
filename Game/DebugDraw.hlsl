#include "Common.hlsli"

void vsMain(in float3 inPosition : POSITION, in float4 inColor : COLOR,
	out float4 outPosition : SV_POSITION, out float4 outColor : COLOR0)
{
	float4 pos = float4(inPosition, 1.0f);
	
	pos = float4(inPosition, 1.0f);
	pos = mul(pos, camera.View);
	pos = mul(pos, camera.Projection);

	outPosition = pos;
	outColor = inColor;
}

float4 psMain(float4 position : SV_POSITION, float4 color : COLOR0) : SV_TARGET
{
	return color;
}
