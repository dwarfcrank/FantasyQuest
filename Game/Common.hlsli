#include "ShaderCommon.h"

cbuffer GlobalCameraConstants : register(b0)
{
	CameraConstants camera;
}

struct VS_Output
{
	float4 Position : SV_POSITION;
	float4 PositionWS : WORLDPOS;
	float4 ShadowPos : SHADOWPOS;
	float3 Normal : NORMAL;
	float4 Color : COLOR0;
	float2 Texcoord : TEXCOORD;
};

struct ShadowVS_Output
{
	float4 Position : SV_POSITION;
};
