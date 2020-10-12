#include "Common.hlsli"
#include "VertexHelpers.hlsli"

cbuffer ShadowCameraConstants : register(b1)
{
	CameraConstants shadowCamera;
};

ByteAddressBuffer vertexBuffer : register(t0);
ByteAddressBuffer instanceBuffer : register(t1);

VS_Output main(uint vIdx : SV_VertexID, uint iIdx : SV_InstanceID)
{
	VS_Output o = (VS_Output)0;

	/*
	matrix world = loadMatrix(instanceBuffer, iIdx * 2 + 0);
	matrix worldInvTranspose = loadMatrix(instanceBuffer, iIdx * 2 + 1);
	*/
	matrix world = loadWorldMatrix(instanceBuffer, iIdx);
	matrix worldInvTranspose = loadWorldInvTranspose(instanceBuffer, iIdx);

	float4 pos = float4(loadPosition(vertexBuffer, vIdx), 1.0f);
	o.PositionWS = mul(pos, world);

	o.Position = mul(pos, world);
	o.Position = mul(o.Position, camera.View);
	o.Position = mul(o.Position, camera.Projection);

	o.ShadowPos = mul(pos, world);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.View);
	o.ShadowPos = mul(o.ShadowPos, shadowCamera.Projection);

	o.Color = loadColor(vertexBuffer, vIdx);

	o.Normal = mul(loadNormal(vertexBuffer, vIdx), (float3x3)worldInvTranspose);

	o.Texcoord = loadTexcoord(vertexBuffer, vIdx);

	return o;
}