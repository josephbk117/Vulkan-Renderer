#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "vulkan/vulkan_core.h"
#include <GLM/glm.hpp>

namespace Utilities
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentationFamily = -1;

		bool IsValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0;
		}
	};

	struct SwapChainInfo
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentationModes;
	};

	struct SwapChainImage
	{
		VkImage image;
		VkImageView imageView;
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 col;
	};

	class Utils
	{
	public:

		static std::vector<char> ReadFile(const std::string& filePath)
		{
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);
			if (!file.is_open())
			{
				throw std::runtime_error("failed to open file : " + filePath);
			}

			const size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();
			return buffer;
		}

		static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags memPropFlags)
		{
			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

			for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
			{
				if ((allowedTypes & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memPropFlags) == memPropFlags)
				{
					return i;
				}
			}
		}
	};
}