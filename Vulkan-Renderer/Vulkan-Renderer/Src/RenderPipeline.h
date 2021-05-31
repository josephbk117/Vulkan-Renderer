#pragma once
#include "RenderPipelinebase.h"
#include <string>
#include "Mesh.h"
namespace Renderer
{
	class RenderPipeline : public RenderPipelineBase
	{
	public:

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);
		VkPipeline GetSecondPipeline() const;
		VkPipelineLayout GetSecondPipelineLayout() const;
		void UpdateUniformBuffers(uint32_t imageIndex, const std::vector<Model>& modelList);
		uint32_t GetModelUniformAlignment() const;
		uint32_t CreateTextureDescriptor(VkImageView textureImage, VkSampler textureSampler);

	private:

		VkPipelineLayout secondPipelineLayout = nullptr;
		VkPipeline secondPipeline = nullptr;

		void CreateDescriptorSetLayout();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreateInputDescriptorSets();
		void CreatePushConstantRange();
		void AllocateDynamicBufferTransferSpace();
	};
}