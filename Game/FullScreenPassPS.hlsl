
Texture2D framebuffer : register(t0);
Texture2D depth : register(t1);

SamplerState fbSampler : register(s0);

float4 main(in float4 position : SV_POSITION, in float2 texcoord : TEXCOORD0) : SV_TARGET
{
	return framebuffer.Sample(fbSampler, texcoord);
}