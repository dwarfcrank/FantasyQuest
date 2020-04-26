#include "Common.hlsli"

VS_Output main(VS_Input v)
{
	VS_Output o = (VS_Output)0;

	o.Position = mul(v.Position, World);
	o.Position = mul(o.Position, View);
	o.Position = mul(o.Position, Projection);

	o.Color = v.Color;

	return o;
}