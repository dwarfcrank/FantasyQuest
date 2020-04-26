#include "Common.hlsli"

float4 main(VS_Output v) : SV_TARGET
{
	return v.Color;
}