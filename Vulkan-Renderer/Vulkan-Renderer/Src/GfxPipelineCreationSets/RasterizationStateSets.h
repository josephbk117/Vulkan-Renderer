#pragma once
#include "vulkan/vulkan_core.h"
namespace Renderer
{
	class RasterizationSets
	{
	public:
		static VkPipelineRasterizationStateCreateInfo Default()
		{
			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_TRUE; // Change to true for correct shadow map generation, Needed GPU feature
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // If enabled all fragments are discarded, Used to get info from other shader stages only
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // Needed GPU feature for other values
			rasterizerCreateInfo.lineWidth = 1.0f; // Needed GPU feature for other values
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE; // Shadow mapping would probably need it

			return rasterizerCreateInfo;
		}

		static VkPipelineRasterizationStateCreateInfo FrontFace()
		{
			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_TRUE; // Change to true for correct shadow map generation, Needed GPU feature
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // If enabled all fragments are discarded, Used to get info from other shader stages only
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // Needed GPU feature for other values
			rasterizerCreateInfo.lineWidth = 1.0f; // Needed GPU feature for other values
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE; // Shadow mapping would probably need it

			return rasterizerCreateInfo;
		}
	};
}