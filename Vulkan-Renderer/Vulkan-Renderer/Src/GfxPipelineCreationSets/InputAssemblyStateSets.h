#pragma once
#include "vulkan/vulkan_core.h"
namespace Renderer
{
	class InputAssemblySets
	{
	public:
		static VkPipelineInputAssemblyStateCreateInfo Default()
		{
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			return inputAssembly;
		}
	};
}