#include "VulkanRenderer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

bool VulkanRenderer::Init(GLFWwindow* window)
{
	this->window = window;

	try
	{
		CreateInstance();
		GetPhysicalDevice();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "\nVulkan Init error : " << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::CleanUp()
{
	vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "NIL";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> instanceExtensions;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	VkResult vkResult = vkCreateInstance(&createInfo, nullptr, &instance);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan instance");
	}
}

void VulkanRenderer::GetPhysicalDevice() const
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount > 0)
	{
		std::vector<VkPhysicalDevice> deviceList(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

		std::cout << "\nNumber of devices found " << deviceCount;

		for (const auto& device : deviceList)
		{
			if (CheckDeviceSuitable(device))
			{
				deviceHandle.physicalDevice = device;
				break;
			}
		}
	}
	else
	{
		throw std::runtime_error("Could not find vulkan compatible GPUs");
	}
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "\nNumber of Extensions supported " << extensionCount;

	for (const auto& props : extensions)
	{
		std::cout << "\n" << props.extensionName;
	}

	for (const auto& checkExtension : *checkExtensions)
	{
		auto found = std::find_if(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& val)
			{
				return strcmp(checkExtension, val.extensionName);
			});

		if (found == std::end(extensions))
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices = GetQueueFamilyIndices(device);
	return indices.IsValid();
}

Utilities::QueueFamilyIndices VulkanRenderer::GetQueueFamilyIndices(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> familyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, familyProps.data());

	for (size_t index = 0; index < familyProps.size(); index++)
	{
		if (familyProps[index].queueCount > 0 && familyProps[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = static_cast<int>(index);
		}

		if (indices.IsValid())
		{
			break;
		}
	}

	return indices;
}
