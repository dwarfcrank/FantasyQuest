cbuffer CameraConstants : register(b0)
{
	matrix View;
	matrix Projection;
};

cbuffer VS_Constants : register(b1)
{
	matrix World;
	matrix WorldInvTranspose;
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
	float3 Normal : NORMAL;
	float4 Color : COLOR0;
};
