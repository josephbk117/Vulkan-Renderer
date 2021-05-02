#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(binding = 0) uniform UboVP
{
	mat4 projection;
	mat4 view;
} uboVP;

layout(binding = 1) uniform UboModel
{
	mat4 model;
} uboModel;

layout(push_constant) uniform PushModel
{
	uint model;
} pushModel;

layout(location = 0) out vec3 outCol;

void main()
{
	outCol = vec3(pushModel.model) * 0.1 * col;
	gl_Position = uboVP.projection * uboVP.view * uboModel.model * vec4(pos, 1.0);
}