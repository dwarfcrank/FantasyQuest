#include "Common.hlsli"
#include "VertexHelpers.hlsli"

ByteAddressBuffer vertexBuffer : register(t0);
ByteAddressBuffer instanceBuffer : register(t1);

float4 main(uint vIdx : SV_VertexID, uint iIdx : SV_InstanceID) : SV_POSITION
{
	matrix world = loadWorldMatrix(instanceBuffer, iIdx);
	float4 position = float4(loadPosition(vertexBuffer, vIdx), 1.0f);

	position = mul(position, world);
	position = mul(position, camera.View);
	position = mul(position, camera.Projection);

	position.z *= position.w;

	return position;
}