#pragma once
#include "vulkan/vulkan_core.h"
namespace Renderer
{
	class MultisampleStateSets
	{
	public:
		static VkPipelineMultisampleStateCreateInfo Default()
		{
			VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {};
			multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			return multiSampleCreateInfo;
		}
	};
}