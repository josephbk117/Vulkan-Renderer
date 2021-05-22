#include "RenderPipeline.h"
#include "Utils.h"
#include <iostream>
#include "ConstantsAndDefines.h"
#include <array>

Renderer::RenderPipeline::~RenderPipeline()
{
	PROFILE_FUNCTION();

	_aligned_free(modelTransferSpace);
	modelTransferSpace = nullptr;

	vkDestroyDescriptorPool(pipelineCreateInfo.device.logicalDevice, inputDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, inputSetLayout, nullptr);

	vkDestroyDescriptorPool(pipelineCreateInfo.device.logicalDevice, samplerDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, samplerSetLayout, nullptr);
	vkDestroyDescriptorPool(pipelineCreateInfo.device.logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, descriptorSetLayout, nullptr);

	for (size_t i = 0; i < vpUniformBuffer.size(); i++)
	{
		vkDestroyBuffer(pipelineCreateInfo.device.logicalDevice, vpUniformBuffer[i], nullptr);
		vkFreeMemory(pipelineCreateInfo.device.logicalDevice, vpUniformBufferMemory[i], nullptr);
		vkDestroyBuffer(pipelineCreateInfo.device.logicalDevice, modelUniformDynamicBuffer[i], nullptr);
		vkFreeMemory(pipelineCreateInfo.device.logicalDevice, modelUniformDynamicBufferMemory[i], nullptr);
	}

	vkDestroyPipeline(pipelineCreateInfo.device.logicalDevice, secondPipeline, nullptr);
	vkDestroyPipelineLayout(pipelineCreateInfo.device.logicalDevice, secondPipelineLayout, nullptr);
	vkDestroyPipeline(pipelineCreateInfo.device.logicalDevice, gfxPipeline, nullptr);
	vkDestroyPipelineLayout(pipelineCreateInfo.device.logicalDevice, pipelineLayout, nullptr);
}

void Renderer::RenderPipeline::Init(const RenderPipelineCreateInfo& pipelineCreateInfo)
{
	PROFILE_FUNCTION();

	this->pipelineCreateInfo = pipelineCreateInfo;

	CreateDescriptorSetLayout();
	CreatePushConstantRange();

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = pipelineCreateInfo.firstPassShaderModule.vertexModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = pipelineCreateInfo.firstPassShaderModule.fragmentModule;
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

	std::array<VkVertexInputAttributeDescription, 4> vertAttributeDescs;
	// Position
	vertAttributeDescs[0].binding = 0;
	vertAttributeDescs[0].location = 0;
	vertAttributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertAttributeDescs[0].offset = offsetof(Vertex, pos);

	// Color
	vertAttributeDescs[1].binding = 0;
	vertAttributeDescs[1].location = 1;
	vertAttributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertAttributeDescs[1].offset = offsetof(Vertex, col);

	// Normal
	vertAttributeDescs[2].binding = 0;
	vertAttributeDescs[2].location = 2;
	vertAttributeDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertAttributeDescs[2].offset = offsetof(Vertex, normal);

	// UV
	vertAttributeDescs[3].binding = 0;
	vertAttributeDescs[3].location = 3;
	vertAttributeDescs[3].format = VK_FORMAT_R32G32_SFLOAT;
	vertAttributeDescs[3].offset = offsetof(Vertex, uv);

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
	
	std::array<VkPipelineColorBlendAttachmentState, 2> colourBlendAttachmentStates = { colorBlendAttachmentState, colorBlendAttachmentState };

	colorBlendingCreateInfo.attachmentCount = static_cast<uint32_t>(colourBlendAttachmentStates.size());
	colorBlendingCreateInfo.pAttachments = colourBlendAttachmentStates.data();

	// Pipeline layout

	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	VkResult vkResult = vkCreatePipelineLayout(pipelineCreateInfo.device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

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
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
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

	vertexShaderCreateInfo.module = pipelineCreateInfo.secondPassShaderModule.vertexModule;
	fragmentShaderCreateInfo.module = pipelineCreateInfo.secondPassShaderModule.fragmentModule;

	VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;

	depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo secondPipelineCreateInfo = {};
	secondPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	secondPipelineCreateInfo.setLayoutCount = 1;
	secondPipelineCreateInfo.pSetLayouts = &inputSetLayout;
	secondPipelineCreateInfo.pushConstantRangeCount = 0;
	secondPipelineCreateInfo.pPushConstantRanges = nullptr;

	vkResult = vkCreatePipelineLayout(pipelineCreateInfo.device.logicalDevice, &secondPipelineCreateInfo, nullptr, &secondPipelineLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	graphicsPipelineCreateInfo.pStages = secondShaderStages;
	graphicsPipelineCreateInfo.layout = secondPipelineLayout;
	graphicsPipelineCreateInfo.subpass = 1;

	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;

	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	// Create second pipeline

	vkResult = vkCreateGraphicsPipelines(pipelineCreateInfo.device.logicalDevice, VK_NULL_HANDLE, 1, 
		&graphicsPipelineCreateInfo, nullptr, &secondPipeline);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	AllocateDynamicBufferTransferSpace();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateInputDescriptorSets();
}

VkPipeline Renderer::RenderPipeline::GetPipeline() const
{
	return gfxPipeline;
}

VkPipeline Renderer::RenderPipeline::GetSecondPipeline() const
{
	return secondPipeline;
}

VkPipelineLayout Renderer::RenderPipeline::GetPipelineLayout() const
{
	return pipelineLayout;
}

VkPipelineLayout Renderer::RenderPipeline::GetSecondPipelineLayout() const
{
	return secondPipelineLayout;
}

VkDescriptorSet& Renderer::RenderPipeline::GetDescriptorSet(uint32_t index)
{
	return (descriptorSets[index]);
}

VkDescriptorSet& Renderer::RenderPipeline::GetSamplerDescriptorSet(uint32_t index)
{
	return samplerDescriptorSets[index];
}

VkDescriptorSet& Renderer::RenderPipeline::GetInputDescriptorSet(uint32_t index)
{
	return inputDescriptorSets[index];
}

void Renderer::RenderPipeline::SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	PROFILE_FUNCTION();

	uboViewProjection.projection = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	uboViewProjection.projection[1][1] *= -1.0f;
}

void Renderer::RenderPipeline::SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec)
{
	uboViewProjection.view = glm::lookAt(location, lookAt, upVec);
}

void Renderer::RenderPipeline::SetModelMatrix(const glm::mat4& mat)
{

}

void Renderer::RenderPipeline::UpdateUniformBuffers(uint32_t imageIndex, const std::vector<Model>& modelList)
{
	PROFILE_FUNCTION();

	// Copy VP data
	void* data = nullptr;
	vkMapMemory(pipelineCreateInfo.device.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(pipelineCreateInfo.device.logicalDevice, vpUniformBufferMemory[imageIndex]);

	// Copy Model data
	size_t totalMeshCount = 0;
	for (size_t i = 0; i < modelList.size(); i++)
	{
		totalMeshCount += modelList[i].GetMeshCount();
		for (size_t j = 0; j < modelList[i].GetMeshCount(); j++)
		{
			UboModel* thisModel = (UboModel*)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
			*thisModel = modelList[i].GetMesh(j)->GetModel();
		}
	}

	vkMapMemory(pipelineCreateInfo.device.logicalDevice, modelUniformDynamicBufferMemory[imageIndex], 0, modelUniformAlignment * totalMeshCount, 0, &data);
	memcpy(data, modelTransferSpace, modelUniformAlignment * totalMeshCount);
	vkUnmapMemory(pipelineCreateInfo.device.logicalDevice, modelUniformDynamicBufferMemory[imageIndex]);
}

uint32_t Renderer::RenderPipeline::GetModelUniformAlignment() const
{
	return modelUniformAlignment;
}

uint32_t Renderer::RenderPipeline::CreateTextureDescriptor(VkImageView textureImage, VkSampler textureSampler)
{
	PROFILE_FUNCTION();

	VkDescriptorSet descriptorSet;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = samplerDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &samplerSetLayout;

	VkResult vkResult = vkAllocateDescriptorSets(pipelineCreateInfo.device.logicalDevice, &allocInfo, &descriptorSet);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate texture descriptor sets!");
	}

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImage;
	imageInfo.sampler = textureSampler;


	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(pipelineCreateInfo.device.logicalDevice, 1, &descriptorWrite, 0, nullptr);

	samplerDescriptorSets.push_back(descriptorSet);

	return static_cast<uint32_t>(samplerDescriptorSets.size()) - 1;
}

void Renderer::RenderPipeline::CreateDescriptorSetLayout()
{
	PROFILE_FUNCTION();
	// VP Binding Info

	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0;
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpLayoutBinding.descriptorCount = 1;
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vpLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	modelLayoutBinding.binding = 1;
	modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelLayoutBinding.descriptorCount = 1;
	modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding , modelLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();

	VkResult vkResult = vkCreateDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

	// Texture sampler descriptor set layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	vkResult = vkCreateDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, &textureLayoutCreateInfo, nullptr, &samplerSetLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

	// Create input attachment image descriptor set layout
	// normal input binding
	VkDescriptorSetLayoutBinding normalInputLayoutBinding = {};
	normalInputLayoutBinding.binding = 0;
	normalInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	normalInputLayoutBinding.descriptorCount = 1;
	normalInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// colour input binding
	VkDescriptorSetLayoutBinding colourInputLayoutBinding = {};
	colourInputLayoutBinding.binding = 1;
	colourInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colourInputLayoutBinding.descriptorCount = 1;
	colourInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// depth input binding
	VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
	depthInputLayoutBinding.binding = 2;
	depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputLayoutBinding.descriptorCount = 1;
	depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> inputBindings = { normalInputLayoutBinding, colourInputLayoutBinding, depthInputLayoutBinding };

	// Create descriptor set layout for input attachments
	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings = inputBindings.data();

	vkResult = vkCreateDescriptorSetLayout(pipelineCreateInfo.device.logicalDevice, &inputLayoutCreateInfo, nullptr, &inputSetLayout);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

}

void Renderer::RenderPipeline::CreateUniformBuffers()
{
	PROFILE_FUNCTION();

	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);
	VkDeviceSize modelBufferSize = static_cast<VkDeviceSize>(modelUniformAlignment) * MAX_OBJECTS;

	vpUniformBuffer.resize(pipelineCreateInfo.swapchainImageCount);
	vpUniformBufferMemory.resize(pipelineCreateInfo.swapchainImageCount);

	modelUniformDynamicBuffer.resize(pipelineCreateInfo.swapchainImageCount);
	modelUniformDynamicBufferMemory.resize(pipelineCreateInfo.swapchainImageCount);

	for (size_t i = 0; i < pipelineCreateInfo.swapchainImageCount; i++)
	{
		Utils::CreateBuffer({ pipelineCreateInfo.device.physicalDevice,
			pipelineCreateInfo.device.logicalDevice, vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vpUniformBuffer[i], &vpUniformBufferMemory[i] });

		Utils::CreateBuffer({ pipelineCreateInfo.device.physicalDevice,
			pipelineCreateInfo.device.logicalDevice, modelBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&modelUniformDynamicBuffer[i], &modelUniformDynamicBufferMemory[i] });
	}
}

void Renderer::RenderPipeline::CreateDescriptorPool()
{
	PROFILE_FUNCTION();

	// Create uniform descriptor pool

	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

	VkDescriptorPoolSize modelPoolSize = {};
	modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(modelUniformDynamicBuffer.size());

	// List of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize, modelPoolSize };

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

	VkResult vkResult = vkCreateDescriptorPool(pipelineCreateInfo.device.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}

	// Create sampler descriptor pool
	// Texture sampler

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS; // TODO : Assumes 1 texture per object, Need to fix this

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	vkResult = vkCreateDescriptorPool(pipelineCreateInfo.device.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}

	// Create input attachment descriptor pool
	// normal attachment pool
	VkDescriptorPoolSize normalInputPoolSize = {};
	normalInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	normalInputPoolSize.descriptorCount = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);

	// colour attachment pool
	VkDescriptorPoolSize colourInputPoolSize = {};
	colourInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colourInputPoolSize.descriptorCount = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);

	// depth attachment pool
	VkDescriptorPoolSize depthInputPoolSize = {};
	depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputPoolSize.descriptorCount = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);

	std::array<VkDescriptorPoolSize, 3> inputPoolSizes = { normalInputPoolSize, colourInputPoolSize, depthInputPoolSize };
	// Create input attachment pool

	VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
	inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	inputPoolCreateInfo.maxSets = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);
	inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
	inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

	vkResult = vkCreateDescriptorPool(pipelineCreateInfo.device.logicalDevice, &inputPoolCreateInfo, nullptr, &inputDescriptorPool);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}

}

void Renderer::RenderPipeline::CreateDescriptorSets()
{
	PROFILE_FUNCTION();

	descriptorSets.resize(pipelineCreateInfo.swapchainImageCount);
	std::vector<VkDescriptorSetLayout> setLayouts(pipelineCreateInfo.swapchainImageCount, descriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo = {};

	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = descriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);
	setAllocateInfo.pSetLayouts = setLayouts.data();

	VkResult vkResult = vkAllocateDescriptorSets(pipelineCreateInfo.device.logicalDevice, &setAllocateInfo, descriptorSets.data());
	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	for (size_t i = 0; i < pipelineCreateInfo.swapchainImageCount; i++)
	{
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = vpUniformBuffer[i];
		vpBufferInfo.offset = 0;
		vpBufferInfo.range = sizeof(UboViewProjection);

		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = descriptorSets[i];
		vpSetWrite.dstBinding = 0;
		vpSetWrite.dstArrayElement = 0;
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vpSetWrite.descriptorCount = 1;
		vpSetWrite.pBufferInfo = &vpBufferInfo;

		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = modelUniformDynamicBuffer[i];
		modelBufferInfo.offset = 0;
		modelBufferInfo.range = modelUniformAlignment;

		VkWriteDescriptorSet modelSetWrite = {};
		modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet = descriptorSets[i];
		modelSetWrite.dstBinding = 1;
		modelSetWrite.dstArrayElement = 0;
		modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelSetWrite.descriptorCount = 1;
		modelSetWrite.pBufferInfo = &modelBufferInfo;

		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite, modelSetWrite };

		vkUpdateDescriptorSets(pipelineCreateInfo.device.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void Renderer::RenderPipeline::CreateInputDescriptorSets()
{
	PROFILE_FUNCTION();
	// Resize array to hold descriptor set for each Swapchain image
	inputDescriptorSets.resize(pipelineCreateInfo.swapchainImageCount);

	std::vector<VkDescriptorSetLayout> setLayouts(pipelineCreateInfo.swapchainImageCount, inputSetLayout);

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = inputDescriptorPool;
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(pipelineCreateInfo.swapchainImageCount);
	setAllocInfo.pSetLayouts = setLayouts.data();

	VkResult result = vkAllocateDescriptorSets(pipelineCreateInfo.device.logicalDevice, &setAllocInfo, inputDescriptorSets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate input attachment descriptor sets");
	}

	// Update each descriptor set with input attachment

	for (size_t i = 0; i < pipelineCreateInfo.swapchainImageCount; i++)
	{
		// Normal attachment descriptor
		VkDescriptorImageInfo normalAttachmentDescriptor = {};
		normalAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalAttachmentDescriptor.imageView = pipelineCreateInfo.normalBufferImageViewPtr->at(i);
		normalAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Normal attachment descriptor write
		VkWriteDescriptorSet normalWrite = {};
		normalWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		normalWrite.dstSet = inputDescriptorSets[i];
		normalWrite.dstBinding = 0;
		normalWrite.dstArrayElement = 0;
		normalWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		normalWrite.descriptorCount = 1;
		normalWrite.pImageInfo = &normalAttachmentDescriptor;

		// Colour attachment descriptor
		VkDescriptorImageInfo colourAttachmentDescriptor = {};
		colourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colourAttachmentDescriptor.imageView = pipelineCreateInfo.colourBufferImageViewPtr->at(i);
		colourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Colour attachment descriptor write
		VkWriteDescriptorSet colourWrite = {};
		colourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colourWrite.dstSet = inputDescriptorSets[i];
		colourWrite.dstBinding = 1;
		colourWrite.dstArrayElement = 0;
		colourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colourWrite.descriptorCount = 1;
		colourWrite.pImageInfo = &colourAttachmentDescriptor;

		// Depth attachment descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView = pipelineCreateInfo.depthBufferImageViewPtr->at(i);
		depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Depth attachment descriptor write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet = inputDescriptorSets[i];
		depthWrite.dstBinding = 2;
		depthWrite.dstArrayElement = 0;
		depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount = 1;
		depthWrite.pImageInfo = &depthAttachmentDescriptor;

		std::array<VkWriteDescriptorSet, 3> setWrites = { normalWrite, colourWrite, depthWrite };

		vkUpdateDescriptorSets(pipelineCreateInfo.device.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);

	}
}

void Renderer::RenderPipeline::CreatePushConstantRange()
{
	PROFILE_FUNCTION();

	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);
}

void Renderer::RenderPipeline::AllocateDynamicBufferTransferSpace()
{
	PROFILE_FUNCTION();

	modelUniformAlignment = (sizeof(UboModel) + pipelineCreateInfo.minUniformBufferOffset - 1) & ~(pipelineCreateInfo.minUniformBufferOffset - 1);

	// Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
	modelTransferSpace = (UboModel*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
}
