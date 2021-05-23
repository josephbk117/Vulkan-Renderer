#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outPos;
layout (location = 1) out vec4 outNorm;
layout (location = 2) out vec4 outCol;

void main()
{
	outCol = texture(textureSampler, inUV);
	outCol.a = 1.0;

	vec3 normalVal = normalize(inNorm);
	outNorm = vec4(normalVal,1);

	outPos = vec4(inPos, 1.0);
}