#include "Common.hlsli"

void vsMain(in float3 inPosition : POSITION, in float4 inColor : COLOR,
	out float4 outPosition : SV_POSITION, out float4 outColor : COLOR0)
{
	float4 pos = float4(inPosition, 1.0f);
	
	pos = mul(pos, camera.View);
	pos = mul(pos, camera.Projection);

	outPosition = pos;
	outColor = inColor;
}

float4 convertColor(uint c)
{
	float4 result = float4(
		(c >> 24) & 0xff,
		(c >> 16) & 0xff,
		(c >>  8) & 0xff,
		(c >>  0) & 0xff
    );

	return result / 255.0f;
}

void vsMain2(in float4 inPosition : POSITION, in uint inColor : COLOR,
	out float4 outPosition : SV_POSITION, out float4 outColor : COLOR0)
{
	float4 pos = float4(inPosition.xyz, 1.0f);
	
	pos = mul(pos, camera.View);
	pos = mul(pos, camera.Projection);

	outPosition = pos;
	outColor = convertColor(inColor);
}

float4 psMain(float4 position : SV_POSITION, float4 color : COLOR0) : SV_TARGET
{
	return color;
}
