#include "ShaderCommon.h"

Texture2D<float4> framebuffer : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= 1920 || dtid.y >= 1080) {
        return;
    }

    output[dtid.xy] = framebuffer[dtid.xy];
}