#include "Common.hlsli"

float4 main(Vertex v, matrix World : WORLD) : SV_POSITION
{
	float4 position = (float4)0;

	position = float4(v.Position, 1.0f);
	position = mul(position, World);
	position = mul(position, camera.View);
	position = mul(position, camera.Projection);

	return position;
}