#ifndef SHADERCOMMON_H
#define SHADERCOMMON_H

#ifdef __cplusplus

#include <DirectXMath.h>

using float2 = DirectX::XMFLOAT2;
using float3 = DirectX::XMFLOAT3;
using float4 = DirectX::XMFLOAT4;
using matrix = DirectX::XMMATRIX;

using uint = UINT;
using uint2 = DirectX::XMUINT2;

#define CB_STRUCT		struct alignas(16)
#define SEMANTIC(s)

#define CONSTANT        constexpr

#else

#define CB_STRUCT		struct
#define SEMANTIC(s)		: s
#define CONSTANT        static const

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
	float LightIntensity;
    uint NumPointLights;
    float3 DirectionalColor;
    float DepthBias;
};

CB_STRUCT GaussianConstants
{
    float2 InputSize;
    float2 OutputSize;
    uint Direction; // 0 = horizontal, 1 = vertical
    uint Upsampling;
};

CB_STRUCT BrightPassConstants
{
    float2 InputSize;
    float2 OutputSize;
    float Threshold;
};

CB_STRUCT KawaseConstants
{
    float2 InputSize;
    float2 OutputSize;
    float KernelSize;
    uint Upsampling;
};

CB_STRUCT BlurConstants
{
    float2 ScreenSize;
    float KernelSize;

    // TODO: remove this, we need a separate shader for the first blur pass anyway
    float TexcoordScale;
    uint BlurPass; // 0 = horizontal, 1 = vertical
};

CB_STRUCT PostProcessConstants
{
    float2 ScreenSize;
	float Exposure;
    uint GammaCorrection;
};

struct Vertex
{
    float3 Position SEMANTIC(POSITION);
    float3 Normal SEMANTIC(NORMAL);
    float4 Color SEMANTIC(COLOR);
    float2 Texcoord SEMANTIC(TEXCOORD);
};

struct PointLight
{
    float4 Position; // .w = linear attenuation
    float4 Color; // .w = quadratic attenuation
    float Intensity;
};

CONSTANT uint TILE_SIZE = 32;

#endif
