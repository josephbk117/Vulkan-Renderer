#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vector>
#include <set>
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
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugMessenger;

	void CreateInstance();
	void CreateValidationDebugMessenger();
	void DestroyValidationDebugMessenger();
	void GetPhysicalDevice()const;
	void CreateLogicalDevice();
	void CreateSurface();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions) const;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice physDevice) const;
	bool CheckDeviceSuitable(VkPhysicalDevice device) const;
	bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers) const;
	QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device) const;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

