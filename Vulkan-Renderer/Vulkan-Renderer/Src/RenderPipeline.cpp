#include "RenderPipeline.h"
#include "Utils.h"
#include <iostream>
#include "ConstantsAndDefines.h"
#include <array>

Renderer::RenderPipeline::~RenderPipeline()
{
	PROFILE_FUNCTION();

	vkDestroyDescriptorPool(pipelineCreateInfo.device.logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, descriptorSetLayout, nullptr);

	for (size_t i = 0; i < uniformBuffer.size(); i++)
	{
		vkDestroyBuffer(pipelineCreateInfo.device.logicalDevice, uniformBuffer[i], nullptr);
		vkFreeMemory(pipelineCreateInfo.device.logicalDevice, uniformBufferMemory[i], nullptr);
	}

	vkDestroyPipeline(pipelineCreateInfo.device.logicalDevice, gfxPipeline, nullptr);
	vkDestroyPipelineLayout(pipelineCreateInfo.device.logicalDevice, pipelineLayout, nullptr);
}

void Renderer::RenderPipeline::Init(const RenderPipelineCreateInfo& pipelineCreateInfo)
{
	PROFILE_FUNCTION();

	this->pipelineCreateInfo = pipelineCreateInfo;

	CreateDescriptorSetLayout();

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = pipelineCreateInfo.vertexModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = pipelineCreateInfo.fragmentModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	VkVertexInputBindingDescription vertexBindingDesc = {};
	vertexBindingDesc.binding = 0;
	vertexBindingDesc.stride = sizeof(Utilities::Vertex);
	/*
	* VK_VERTEX_INPUT_RATE_VERTEX :If instance enabled the draw one instance at a time instead of one vertex for each mesh at the same time
	* VK_VERTEX_INPUT_RATE_INSTANCE : Other way around
	* */
	vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertAttributeDescs;
	vertAttributeDescs[0].binding = 0;
	vertAttributeDescs[0].location = 0;
	vertAttributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertAttributeDescs[0].offset = offsetof(Vertex, pos);

	vertAttributeDescs[1].binding = 0;
	vertAttributeDescs[1].location = 1;
	vertAttributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertAttributeDescs[1].offset = offsetof(Vertex, col);

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};

	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDesc;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertAttributeDescs.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = vertAttributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(pipelineCreateInfo.extent.width);
	viewport.height = static_cast<float>(pipelineCreateInfo.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = pipelineCreateInfo.extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_TRUE; // Change to true for correct shadow map generation, Needed GPU feature
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // If enabled all fragments are discarded, Used to get info from other shader stages only
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // Needed GPU feature for other values
	rasterizerCreateInfo.lineWidth = 1.0f; // Needed GPU feature for other values
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE; // Shadow mapping would probably need it

	VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {};
	multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;


	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult vkResult = vkCreatePipelineLayout(pipelineCreateInfo.device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multiSampleCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = pipelineCreateInfo.renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	vkResult = vkCreateGraphicsPipelines(pipelineCreateInfo.device.logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &gfxPipeline);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
}

VkPipeline Renderer::RenderPipeline::GetPipeline() const
{
	return gfxPipeline;
}

VkPipelineLayout Renderer::RenderPipeline::GetPipelineLayout() const
{
	return pipelineLayout;
}

VkDescriptorSet& Renderer::RenderPipeline::GetDescriptorSet(uint32_t index)
{
	return (descriptorSets[index]);
}

void Renderer::RenderPipeline::SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	PROFILE_FUNCTION();

	mvp.projection = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	mvp.projection[1][1] *= -1.0f;
}

void Renderer::RenderPipeline::SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec)
{
	mvp.view = glm::lookAt(location, lookAt, upVec);
}

void Renderer::RenderPipeline::SetModelMatrix(const glm::mat4& mat)
{
	mvp.model = mat;
}

void Renderer::RenderPipeline::UpdateUniformBuffer(uint32_t imageIndex)
{
	PROFILE_FUNCTION();

	void* data = nullptr;
	vkMapMemory(pipelineCreateInfo.device.logicalDevice, uniformBufferMemory[imageIndex], 0, sizeof(MVP), 0, &data);
	memcpy(data, &mvp, sizeof(MVP));
	vkUnmapMemory(pipelineCreateInfo.device.logicalDevice, uniformBufferMemory[imageIndex]);
}

void Renderer::RenderPipeline::CreateDescriptorSetLayout()
{
	PROFILE_FUNCTION();
	// MVP Binding Info

	VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
	mvpLayoutBinding.binding = 0;
	mvpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	mvpLayoutBinding.descriptorCount = 1;
	mvpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mvpLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = 1;
	layoutCreateInfo.pBindings = &mvpLayoutBinding;

	VkResult vkResult = vkCreateDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::RenderPipeline::CreateUniformBuffers()
{
	PROFILE_FUNCTION();

	VkDeviceSize bufferSize = sizeof(MVP);
	uniformBuffer.resize(pipelineCreateInfo.swapchainImageCount);
	uniformBufferMemory.resize(pipelineCreateInfo.swapchainImageCount);

	for (size_t i = 0; i < pipelineCreateInfo.swapchainImageCount; i++)
	{
		Utils::CreateBuffer({ pipelineCreateInfo.device.physicalDevice,
			pipelineCreateInfo.device.logicalDevice, bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer[i], &uniformBufferMemory[i] });
	}
}

void Renderer::RenderPipeline::CreateDescriptorPool()
{
	PROFILE_FUNCTION();

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(uniformBuffer.size());

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(uniformBuffer.size());
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = &poolSize;

	VkResult vkResult = vkCreateDescriptorPool(pipelineCreateInfo.device.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void Renderer::RenderPipeline::CreateDescriptorSets()
{
	PROFILE_FUNCTION();

	descriptorSets.resize(uniformBuffer.size());
	std::vector<VkDescriptorSetLayout> setLayouts(uniformBuffer.size(), descriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo = {};

	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = descriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(uniformBuffer.size());
	setAllocateInfo.pSetLayouts = setLayouts.data();

	VkResult vkResult = vkAllocateDescriptorSets(pipelineCreateInfo.device.logicalDevice, &setAllocateInfo, descriptorSets.data());
	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	for (size_t i = 0; i < uniformBuffer.size(); i++)
	{
		VkDescriptorBufferInfo mvpBufferInfo = {};
		mvpBufferInfo.buffer = uniformBuffer[i];
		mvpBufferInfo.offset = 0;
		mvpBufferInfo.range = sizeof(MVP);

		VkWriteDescriptorSet mvpSetWrite = {};
		mvpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mvpSetWrite.dstSet = descriptorSets[i];
		mvpSetWrite.dstBinding = 0;
		mvpSetWrite.dstArrayElement = 0;
		mvpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mvpSetWrite.descriptorCount = 1;
		mvpSetWrite.pBufferInfo = &mvpBufferInfo;

		vkUpdateDescriptorSets(pipelineCreateInfo.device.logicalDevice, 1, &mvpSetWrite, 0, nullptr);
	}
}
