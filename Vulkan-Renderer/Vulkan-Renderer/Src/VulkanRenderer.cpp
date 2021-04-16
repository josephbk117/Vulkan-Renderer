#include "VulkanRenderer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "ConstantsAndDefines.h"

bool VulkanRenderer::Init(GLFWwindow* window)
{
	this->window = window;

	try
	{
		CreateInstance();
		CreateValidationDebugMessenger();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
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
	DestroyValidationDebugMessenger();
	vkDestroySwapchainKHR(deviceHandle.logicalDevice, swapChain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(deviceHandle.logicalDevice, nullptr);
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
	appInfo.apiVersion = VK_API_VERSION_1_2;

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

	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	std::vector<const char*> validationLayers = VALIDATION_LAYERS;

	if (!CheckValidationLayerSupport(&validationLayers))
	{
		throw std::runtime_error("No required validation layer is supported");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();

	VkResult vkResult = vkCreateInstance(&createInfo, nullptr, &instance);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan instance");
	}
}

void VulkanRenderer::CreateValidationDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, &createInfo, nullptr, &debugMessenger);
	}
	else
	{
		throw std::runtime_error("Could not create validation debug messenger");
	}
}

void VulkanRenderer::DestroyValidationDebugMessenger()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, nullptr);
	}
	else
	{
		throw std::runtime_error("Failed to destroy validation debug messenger");
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

void VulkanRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indices = GetQueueFamilyIndices(deviceHandle.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	VkResult vkResult = vkCreateDevice(deviceHandle.physicalDevice, &deviceCreateInfo, nullptr, &deviceHandle.logicalDevice);
	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan logical device");
	}

	vkGetDeviceQueue(deviceHandle.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(deviceHandle.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

void VulkanRenderer::CreateSwapChain()
{
	SwapChainInfo swapChainInfo = GetSwapChainDetails(deviceHandle.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = GetSuitableSurfaceFormat(swapChainInfo.surfaceFormats);
	VkPresentModeKHR presentationMode = GetSuitablePresentationMode(swapChainInfo.presentationModes);
	VkExtent2D extents = GetSuitableSwapExtent(swapChainInfo.surfaceCapabilities);

	uint32_t imageCount = swapChainInfo.surfaceCapabilities.minImageCount + 1;

	if (swapChainInfo.surfaceCapabilities.maxImageCount > 0 && swapChainInfo.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainInfo.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.imageExtent = extents;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swapChainInfo.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;

	QueueFamilyIndices indices = GetQueueFamilyIndices(deviceHandle.physicalDevice);

	if (indices.graphicsFamily != indices.presentationFamily)
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentationFamily };

		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult vkResult = vkCreateSwapchainKHR(deviceHandle.logicalDevice, &swapChainCreateInfo, nullptr, &swapChain);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain");
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extents;

}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions) const
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

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice physDevice) const
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
	{
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, extensions.data());

	for (const auto& deviceExtension : DEVICE_EXTENSIONS)
	{
		auto found = std::find_if(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& val)
			{
				return strcmp(deviceExtension, val.extensionName);
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
	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		SwapChainInfo swapChainInfo = GetSwapChainDetails(device);
		swapChainValid = !swapChainInfo.presentationModes.empty() && !swapChainInfo.surfaceFormats.empty();
	}

	return indices.IsValid() && extensionsSupported && swapChainValid;
}

bool VulkanRenderer::CheckValidationLayerSupport(std::vector<const char*>* validationLayers) const
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	std::cout << "\nValidation Layers supported : " << layerCount;

	for (const auto& layer : *validationLayers)
	{
		auto found = std::find_if(availableLayers.begin(), availableLayers.end(), [&](const VkLayerProperties& val)
			{
				return strcmp(layer, val.layerName);
			});

		if (found == std::end(availableLayers))
		{
			return false;
		}
	}

	return true;
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

		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentationSupport);

		if (familyProps[index].queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = static_cast<int>(index);
		}

		if (indices.IsValid())
		{
			break;
		}
	}

	return indices;
}

Utilities::SwapChainInfo VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device) const
{
	SwapChainInfo swapChainInfo;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainInfo.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		swapChainInfo.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainInfo.surfaceFormats.data());
	}

	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount > 0)
	{
		swapChainInfo.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainInfo.presentationModes.data());
	}

	return swapChainInfo;
}

VkSurfaceFormatKHR VulkanRenderer::GetSuitableSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) const
{
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : surfaceFormats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return surfaceFormats[0];
}

VkPresentModeKHR VulkanRenderer::GetSuitablePresentationMode(const std::vector<VkPresentModeKHR>& presentationMode) const
{
	for (const auto& presentationMode : presentationMode)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::GetSuitableSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		extent.width = std::min(surfaceCapabilities.maxImageExtent.width, extent.width);
		extent.width = std::max(surfaceCapabilities.minImageExtent.width, extent.width);
		extent.height = std::min(surfaceCapabilities.maxImageExtent.height, extent.height);
		extent.height = std::max(surfaceCapabilities.minImageExtent.height, extent.height);

		return extent;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:/*std::cerr << " [Verbose]:"; break*/ return VK_FALSE;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:std::cerr << "Validation [Info]:"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:std::cerr << "Validation [Warning]:"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:	std::cerr << "Validation [Error]:"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:break;
	}

	std::cerr << pCallbackData->pMessage;
	return VK_FALSE;
}
