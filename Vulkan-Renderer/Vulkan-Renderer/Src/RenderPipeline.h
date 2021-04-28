#pragma once
#include "VulkanRenderer.h"
#include <string>
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
		};

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);
		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
		void SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
		void SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec);
		void SetModelMatrix(const glm::mat4& mat);
		void UpdateUniformBuffer(uint32_t imageIndex);

	private:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout;
		VkPipeline gfxPipeline;

		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkBuffer> uniformBuffer;
		std::vector<VkDeviceMemory> uniformBufferMemory;
		MVP mvp;

		void CreateDescriptorSetLayout();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
	};
}