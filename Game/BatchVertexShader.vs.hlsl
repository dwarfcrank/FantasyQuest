#include "Common.hlsli"

cbuffer ShadowCameraConstants : register(b1)
{
	CameraConstants shadowCamera;
};

VS_Output main(Vertex v, matrix World : WORLD, matrix WorldInvTranspose : WORLD_IT)
{
	VS_Output o = (VS_Output)0;

	float4 pos = float4(v.Position, 1.0f);
	o.PositionWS = mul(pos, World);

	o.Position = mul(pos, World);
	o.Position = mul(o.Position, camera.View);
	o.Position = mul(o.Position, camera.Projection);

	o.ShadowPos = mul(pos, World);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.View);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.Projection);

	o.Color = v.Color;

	o.Normal = mul(v.Normal, (float3x3)WorldInvTranspose);

	o.Texcoord = v.Texcoord;

	return o;
}