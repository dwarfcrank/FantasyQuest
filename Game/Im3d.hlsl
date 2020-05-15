#include "Common.hlsli"

static const float2 screenSize = float2(1920.0f, 1080.0f);
static const float antialiasing = 2.0f;

struct VSInput
{
	float4 position : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	linear float4 position : SV_POSITION;
	linear float4 color : COLOR;

	noperspective float size : SIZE;
	noperspective float edgeDistance : EDGEDISTANCE;
};

VSOutput vsMain(VSInput input)
{
	VSOutput output;

	output.position = float4(input.position.xyz, 1.0f);
	output.position = mul(output.position, camera.View);
	output.position = mul(output.position, camera.Projection);

	output.color = input.color.abgr;
	output.color.a *= smoothstep(0.0f, 1.0f, input.position.w / antialiasing);

	output.size = max(input.position.w, antialiasing);
	output.edgeDistance = 0.0f;

	return output;
}

VSOutput vsTriangles(VSInput input)
{
	VSOutput output;

	output.position = float4(input.position.xyz, 1.0f);
	output.position = mul(output.position, camera.View);
	output.position = mul(output.position, camera.Projection);

	output.color = input.color.abgr;

	output.size = max(input.position.w, antialiasing);
	output.edgeDistance = 0.0f;

	return output;
}

[maxvertexcount(4)]
void gsLines(line VSOutput input[2], inout TriangleStream<VSOutput> output)
{
	float2 pos[2] = {
		input[0].position.xy / input[0].position.w,
		input[1].position.xy / input[1].position.w,
	};

	float2 direction = pos[0] - pos[1];
	direction = normalize(direction * float2(1.0f, screenSize.y / screenSize.x));
	
	float2 normals[2] = {
		float2(-direction.y, direction.x) * input[0].size / screenSize,
		float2(-direction.y, direction.x) * input[1].size / screenSize,
	};

	VSOutput o;

	o.size = input[0].size;
	o.color = input[0].color;

	o.position = float4(pos[0].xy - normals[0], input[0].position.zw);
	o.position.xy *= o.position.w;
	o.edgeDistance = -o.size;
	output.Append(o);

	o.position = float4(pos[0].xy + normals[0], input[0].position.zw);
	o.position.xy *= o.position.w;
	o.edgeDistance = o.size;
	output.Append(o);

	o.size = input[1].size;
	o.color = input[1].color;

	o.position = float4(pos[1].xy - normals[1], input[1].position.zw);
	o.position.xy *= o.position.w;
	o.edgeDistance = -o.size;
	output.Append(o);

	o.position = float4(pos[1].xy + normals[1], input[1].position.zw);
	o.position.xy *= o.position.w;
	o.edgeDistance = o.size;
	output.Append(o);
}

float4 psMain(VSOutput v) : SV_TARGET
{
	float t = abs(v.edgeDistance / v.size);
    float4 color = v.color;
	color.a *= smoothstep(1.0f, 1.0f - (antialiasing / v.size), t);

	return color;
}

float4 psTriangles(VSOutput v) : SV_TARGET
{
	return v.color;
}
