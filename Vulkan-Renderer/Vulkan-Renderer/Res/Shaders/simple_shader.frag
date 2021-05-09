#version 450

layout(location = 0) in vec3 inCol;
layout(location = 1) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outCol;

void main()
{
	outCol = texture(textureSampler, inUV) * vec4(inCol, 1.0);
}