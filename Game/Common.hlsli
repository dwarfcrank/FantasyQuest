/*
cbuffer CameraConstants : register(b0)
{
	matrix View;
	matrix Projection;
};
*/

#include "ShaderCommon.h"

cbuffer GlobalCameraConstants : register(b0)
{
	CameraConstants camera;
}

struct VS_Input
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
};

struct VS_Output
{
	float4 Position : SV_POSITION;
	float4 PositionWS : TEXCOORD0;
	float4 ShadowPos : TEXCOORD1;
	float3 Normal : NORMAL;
	float4 Color : COLOR0;
};

struct ShadowVS_Output
{
	float4 Position : SV_POSITION;
};
