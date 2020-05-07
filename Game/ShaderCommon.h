#ifndef SHADERCOMMON_H
#define SHADERCOMMON_H

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

struct Vertex
{
	float3 Position SEMANTIC(POSITION);
	float3 Normal SEMANTIC(NORMAL);
	float4 Color SEMANTIC(COLOR);
};

struct PointLight
{
    float4 Position; // .w = linear attenuation
    float4 Color; // .w = quadratic attenuation
};

#endif
