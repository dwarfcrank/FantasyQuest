#include "Common.hlsli"

cbuffer VS_Constants : register(b1)
{
	matrix World;
	matrix WorldInvTranspose;
};

ShadowVS_Output main(VS_Input v)
{
	ShadowVS_Output o = (ShadowVS_Output)0;

	o.Position = float4(v.Position, 1.0f);
	o.Position = mul(o.Position, World);
	o.Position = mul(o.Position, View);
	o.Position = mul(o.Position, Projection);

	return o;
}