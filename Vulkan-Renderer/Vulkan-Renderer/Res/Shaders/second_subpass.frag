#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputPos; // Position output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal; // Normal output from subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputColour; // Colour output from subpass 1
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inputDepth; // Depth output from subpass 1

layout(location = 0) out vec4 outCol;

void main()
{
	vec3 normal = subpassLoad(inputNormal).rgb;
	normal = normal * 0.5 + 0.5;
	outCol.rgb = subpassLoad(inputColour).rgb;
	outCol.rgb *= clamp(dot(normal, vec3(0,1,0)), 0.0, 1.0);

	float lightMul = pow(1.0/distance(subpassLoad(inputPos).rgb, vec3(20, 40, 0)), 1.25);
	lightMul *= 35.0;
	lightMul = clamp(lightMul, 0.0, 1.0);
	outCol.rgb *= lightMul;

	float depth = subpassLoad(inputDepth).r;

	const float upperBound = 1.0f;
	const float lowerBound = 0.999f;
	float depthScaled = 1.0f - ((depth - lowerBound)/(upperBound - lowerBound));

	outCol.rgb *= depthScaled;

	outCol.a = 1.0;
}