#pragma once
#include "VulkanRenderer.h"
#include <string>
#include "Mesh.h"
namespace Renderer
{
	class RenderPipeline
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
			std::vector<VkImageView>* normalBufferImageViewPtr = nullptr;
			std::vector<VkImageView>* albedoBufferImageViewPtr = nullptr;
			std::vector<VkImageView>* depthBufferImageViewPtr = nullptr;
		};

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);
		VkPipeline GetPipeline() const;
		VkPipeline GetSecondPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;
		VkPipelineLayout GetSecondPipelineLayout() const;
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
		VkDescriptorSet& GetSamplerDescriptorSet(uint32_t index);
		VkDescriptorSet& GetInputDescriptorSet(uint32_t index);
		void SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
		void SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec);
		void SetModelMatrix(const glm::mat4& mat);
		void UpdateUniformBuffers(uint32_t imageIndex, const std::vector<Model>& modelList);
		uint32_t GetModelUniformAlignment() const;
		uint32_t CreateTextureDescriptor(VkImageView textureImage, VkSampler textureSampler);

	private:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipelineLayout secondPipelineLayout = nullptr;
		VkPipeline gfxPipeline = nullptr;
		VkPipeline secondPipeline = nullptr;

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

		void CreateDescriptorSetLayout();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreateInputDescriptorSets();
		void CreatePushConstantRange();

		void AllocateDynamicBufferTransferSpace();
	};
}