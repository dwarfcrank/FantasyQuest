#include "Common.hlsli"

cbuffer VS_Constants : register(b1)
{
	matrix World;
	matrix WorldInvTranspose;
};

VS_Output main(VS_Input v)
{
	VS_Output o = (VS_Output)0;

	float4 pos = float4(v.Position, 1.0f);
	o.PositionWS = mul(pos, World);
	o.Position = mul(pos, World);
	o.Position = mul(o.Position, View);
	o.Position = mul(o.Position, Projection);

	o.Color = v.Color;

	o.Normal = mul(v.Normal, (float3x3)WorldInvTranspose);

	return o;
}