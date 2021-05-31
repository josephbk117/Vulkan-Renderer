#pragma once
#include "../RenderPipelineBase.h"
namespace Renderer
{
	class ViewportStateCreationSets
	{
	public:
		static VkPipelineViewportStateCreateInfo& Default(VkViewport* viewport, VkRect2D* scissor)
		{
			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = viewport;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = scissor;

			return viewportStateCreateInfo;
		}
	};
}