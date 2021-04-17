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
		};

		RenderPipeline() = default;
		~RenderPipeline();
		void Init(const RenderPipelineCreateInfo& pipelineCreateInfo);

	private:

		RenderPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout pipelineLayout;
	};
}