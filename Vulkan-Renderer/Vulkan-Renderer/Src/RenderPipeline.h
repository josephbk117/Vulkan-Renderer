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
			VkDevice device;
			VkRenderPass renderPass;
		};

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);
		VkPipeline GetPipeline() const;

	private:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout;
		VkPipeline gfxPipeline;
	};
}