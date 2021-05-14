#version 450

layout(location = 0) in vec3 inCol;
layout(location = 1) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outCol;

void main()
{
	vec3 normalVal = normalize(inCol) * 0.5 + 0.5;
	outCol = texture(textureSampler, inUV) * dot(normalVal, vec3(0,1,0));
	outCol.a = 1.0;
}