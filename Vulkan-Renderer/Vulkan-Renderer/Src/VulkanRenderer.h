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
	VkDebugUtilsMessengerEXT debugMessenger;

	void CreateInstance();
	void CreateValidationDebugMessenger();
	void DestroyValidationDebugMessenger();
	void GetPhysicalDevice()const;
	void CreateLogicalDevice();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device)const;
	bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers) const;
	QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device)const;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,	void* pUserData);
};

