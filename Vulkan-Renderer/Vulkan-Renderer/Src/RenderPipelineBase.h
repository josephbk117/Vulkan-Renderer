#pragma once
#include "VulkanRenderer.h"

namespace Renderer
{
	class RenderPipelineBase
	{
	public:

		struct ShaderModuleSet
		{
			VkShaderModule vertexModule;
			VkShaderModule fragmentModule;
		};

		struct RenderPipelineCreateInfo
		{
			ShaderModuleSet firstPassShaderModule;
			ShaderModuleSet secondPassShaderModule;
			VkExtent2D extent;
			DeviceHandle device;
			VkRenderPass renderPass;
			size_t swapchainImageCount = 0;
			VkDeviceSize minUniformBufferOffset;
			std::vector<VkImageView>* positionBufferImageViewPtr = nullptr;
			std::vector<VkImageView>* normalBufferImageViewPtr = nullptr;
			std::vector<VkImageView>* albedoBufferImageViewPtr = nullptr;
			std::vector<VkImageView>* depthBufferImageViewPtr = nullptr;
		};

		virtual ~RenderPipelineBase() {};
		virtual void Init(const RenderPipelineCreateInfo& pipelineCreateInfo) = 0;
		virtual VkPipeline GetPipeline() const;
		virtual VkPipelineLayout GetPipelineLayout() const;
		virtual VkDescriptorSet& GetDescriptorSet(uint32_t index);
		virtual VkDescriptorSet& GetSamplerDescriptorSet(uint32_t index);
		virtual VkDescriptorSet& GetInputDescriptorSet(uint32_t index);
		virtual void SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
		virtual void SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec);
		virtual void UpdateUniformBuffers(uint32_t imageIndex, const std::vector<Model>& modelList) = 0;
		virtual uint32_t GetModelUniformAlignment() const = 0;
		virtual uint32_t CreateTextureDescriptor(VkImageView textureImage, VkSampler textureSampler) = 0;

	protected:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline gfxPipeline = nullptr;

		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkDescriptorSetLayout samplerSetLayout = nullptr;
		VkDescriptorSetLayout inputSetLayout = nullptr;
		VkPushConstantRange pushConstantRange;

		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorPool samplerDescriptorPool = nullptr;
		VkDescriptorPool inputDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> descriptorSets; // We need as many of these as there are swapchain images
		std::vector<VkDescriptorSet> samplerDescriptorSets; // We need one of these per image
		std::vector<VkDescriptorSet> inputDescriptorSets; // We need one of these per image

		uint32_t modelUniformAlignment;
		UboModel* modelTransferSpace = nullptr;

		std::vector<VkBuffer> vpUniformBuffer;
		std::vector<VkDeviceMemory> vpUniformBufferMemory;

		std::vector<VkBuffer> modelUniformDynamicBuffer;
		std::vector<VkDeviceMemory> modelUniformDynamicBufferMemory;

		UboViewProjection uboViewProjection;

		virtual void CreateDescriptorSetLayout() = 0;
		virtual void CreateUniformBuffers() = 0;
		virtual void CreateDescriptorPool() = 0;
		virtual void CreateDescriptorSets() = 0;
		virtual void CreateInputDescriptorSets() = 0;
		virtual void CreatePushConstantRange() = 0;
		virtual void AllocateDynamicBufferTransferSpace() = 0;

	private:
	};
}