#ifndef VERTEXHELPERS_H
#define VERTEXHELPERS_H

#define VERTEX_STRIDE	48
#define OFFSET_POSITION 0
#define OFFSET_NORMAL	12
#define OFFSET_COLOR	24
#define OFFSET_TEXCOORD	40

#define INSTANCE_STRIDE				128
#define OFFSET_WORLD				0
#define OFFSET_WORLD_INV_TRANSPOSE	64

float3 loadPosition(ByteAddressBuffer vb, uint idx)
{
	return asfloat(vb.Load3(idx * VERTEX_STRIDE + OFFSET_POSITION));
}

float3 loadNormal(ByteAddressBuffer vb, uint idx)
{
	return asfloat(vb.Load3(idx * VERTEX_STRIDE + OFFSET_NORMAL));
}

float4 loadColor(ByteAddressBuffer vb, uint idx)
{
	return asfloat(vb.Load4(idx * VERTEX_STRIDE + OFFSET_COLOR));
}

float2 loadTexcoord(ByteAddressBuffer vb, uint idx)
{
	return asfloat(vb.Load2(idx * VERTEX_STRIDE + OFFSET_TEXCOORD));
}

matrix loadMatrix(ByteAddressBuffer vb, uint offset)
{
	float4 c0 = asfloat(vb.Load4(offset + 0));
	float4 c1 = asfloat(vb.Load4(offset + 16));
	float4 c2 = asfloat(vb.Load4(offset + 32));
	float4 c3 = asfloat(vb.Load4(offset + 48));

	return transpose(matrix(c0, c1, c2, c3));
}

matrix loadWorldMatrix(ByteAddressBuffer ib, uint idx)
{
	return loadMatrix(ib, idx * INSTANCE_STRIDE + OFFSET_WORLD);
}

matrix loadWorldInvTranspose(ByteAddressBuffer ib, uint idx)
{
	return loadMatrix(ib, idx * INSTANCE_STRIDE + OFFSET_WORLD_INV_TRANSPOSE);
}

#endif
