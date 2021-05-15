#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour; // Colour output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth; // Depth output from subpass 1

layout(location = 0) out vec4 outCol;

void main()
{
	outCol = subpassLoad(inputColour).rgba;
	float depth = subpassLoad(inputDepth).r;

	const float upperBound = 1.0f;
	const float lowerBound = 0.999f;
	float depthScaled = 1.0f - ((depth - lowerBound)/(upperBound - lowerBound));

	outCol.rgb *= depthScaled;
}