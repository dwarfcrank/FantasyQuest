#pragma once

#ifdef __cplusplus

#include <DirectXMath.h>

using float2 = DirectX::XMFLOAT2;
using float3 = DirectX::XMFLOAT3;
using float4 = DirectX::XMFLOAT4;
using matrix = DirectX::XMMATRIX;

using uint = UINT;

#define CB_STRUCT		struct alignas(16)
#define SEMANTIC(s)

#else

#define CB_STRUCT		struct
#define SEMANTIC(s)		: s

#endif

CB_STRUCT CameraConstants
{
	matrix View;
	matrix Projection;
};

CB_STRUCT RenderableConstants
{
    matrix World;
    matrix WorldInvTranspose;
};

CB_STRUCT PSConstants
{
	float3 LightDir;
    uint NumPointLights;
};

/*

struct alignas(16) PSConstantBuffer
{
    XMFLOAT3 LightPosition;
    UINT NumPointLights;
};

cbuffer CameraConstants : register(b0)
{
	matrix View;
	matrix Projection;
};

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
*/
