#version 450

layout(location = 0) in vec3 inCol;

layout (location = 0) out vec4 outCol;

void main()
{
	outCol = vec4(inCol, 1.0);
}