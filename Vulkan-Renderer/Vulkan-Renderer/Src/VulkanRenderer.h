#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vector>
#include "Utils.h"

using namespace Utilities;

class VulkanRenderer
{
public:
	bool Init(GLFWwindow* window);
	void CleanUp();

private:

	struct DeviceHandle
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	}
	mutable deviceHandle;

	GLFWwindow* window = nullptr;
	VkInstance instance;
	VkQueue graphicsQueue;

	void CreateInstance();
	void GetPhysicalDevice()const;
	void CreateLogicalDevice();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device)const;
	QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device)const;
};

