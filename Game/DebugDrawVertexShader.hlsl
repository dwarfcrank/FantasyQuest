#include "Common.hlsli"

struct Output
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

Output main(float3 position : POSITION, float4 color : COLOR)
{
	Output o = (Output)0;
	
	o.position = float4(position, 1.0f);
	o.position = mul(o.position, View);
	o.position = mul(o.position, Projection);
	o.color = color;

	return o;
}