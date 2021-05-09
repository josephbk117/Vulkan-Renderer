#pragma once
#include "VulkanRenderer.h"
#include <string>
#include "Mesh.h"
namespace Renderer
{
	class RenderPipeline
	{
	public:
		struct RenderPipelineCreateInfo
		{
			VkShaderModule vertexModule;
			VkShaderModule fragmentModule;
			VkExtent2D extent;
			DeviceHandle device;
			VkRenderPass renderPass;
			size_t swapchainImageCount = 0;
			VkDeviceSize minUniformBufferOffset;
		};

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);
		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
		VkDescriptorSet& GetSamplerDescriptorSet(uint32_t index);
		void SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
		void SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec);
		void SetModelMatrix(const glm::mat4& mat);
		void UpdateUniformBuffers(uint32_t imageIndex, const std::vector<Mesh>& meshList);
		uint32_t GetModelUniformAlignment() const;
		uint32_t CreateTextureDescriptor(VkImageView textureImage, VkSampler textureSampler);

	private:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline gfxPipeline = nullptr;

		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkDescriptorSetLayout samplerSetLayout = nullptr;
		VkPushConstantRange pushConstantRange;

		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorPool samplerDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> descriptorSets; // We need as many of these as there are swapchain images
		std::vector<VkDescriptorSet> samplerDescriptorSets; // We need one of these per image

		uint32_t modelUniformAlignment;
		UboModel* modelTransferSpace = nullptr;

		std::vector<VkBuffer> vpUniformBuffer;
		std::vector<VkDeviceMemory> vpUniformBufferMemory;

		std::vector<VkBuffer> modelUniformDynamicBuffer;
		std::vector<VkDeviceMemory> modelUniformDynamicBufferMemory;

		UboViewProjection uboViewProjection;

		void CreateDescriptorSetLayout();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreatePushConstantRange();

		void AllocateDynamicBufferTransferSpace();
	};
}