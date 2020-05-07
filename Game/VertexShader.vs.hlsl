#include "Common.hlsli"

cbuffer VS_Constants : register(b1)
{
	/*
	matrix World;
	matrix WorldInvTranspose;
	*/
	RenderableConstants rc;
};

cbuffer ShadowCameraConstants : register(b2)
{
	/*
	matrix ShadowView;
	matrix ShadowProjection;
	*/
	CameraConstants shadowCamera;
};

VS_Output main(VS_Input v)
{
	VS_Output o = (VS_Output)0;

	float4 pos = float4(v.Position, 1.0f);
	o.PositionWS = mul(pos, rc.World);

	o.Position = mul(pos, rc.World);
	o.Position = mul(o.Position, camera.View);
	o.Position = mul(o.Position, camera.Projection);

	o.ShadowPos = mul(pos, rc.World);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.View);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.Projection);

	o.Color = v.Color;

	o.Normal = mul(v.Normal, (float3x3)rc.WorldInvTranspose);

	return o;
}