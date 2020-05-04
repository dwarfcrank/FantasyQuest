static const float2 vertices[4] = {
    float2(1.0f, -1.0f),    // lower right
    float2(-1.0f, -1.0f),   // lower left
    float2(1.0f, 1.0f),     // upper right
    float2(-1.0f, 1.0f),    // upper left
};

static const float2 texcoords[4] = {
    float2(1.0f, 1.0f), // lower right
    float2(0.0f, 1.0f), // lower left
    float2(1.0f, 0.0f), // upper right
    float2(0.0f, 0.0f), // upper left
};

void main(in uint i : SV_VERTEXID, out float4 position : SV_POSITION, out float2 texcoord : TEXCOORD0)
{
    position = float4(vertices[i], 0.1f, 1.0f);
    texcoord = texcoords[i];
}