cbuffer CameraConstants : register(b0)
{
	matrix View;
	matrix Projection;
};

cbuffer VS_Constants : register(b1)
{
	matrix World;
};

struct VS_Input
{
	float4 Position : POSITION;
	float4 Color : COLOR;
};

struct VS_Output
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
};
