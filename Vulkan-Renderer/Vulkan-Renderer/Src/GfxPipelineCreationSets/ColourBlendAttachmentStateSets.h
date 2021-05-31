#pragma once
#include "vulkan/vulkan_core.h"
namespace Renderer
{
	class ColourBlendAttachmentStateSets
	{
	public:
		static VkPipelineColorBlendAttachmentState Default()
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
			colorBlendAttachmentState.blendEnable = VK_TRUE;
			colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

			return colorBlendAttachmentState;
		}
	};
}