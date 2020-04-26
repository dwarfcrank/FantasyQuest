cbuffer VS_Constants : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
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
