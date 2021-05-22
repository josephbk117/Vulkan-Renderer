#include "VulkanRenderer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <string>
#include "ConstantsAndDefines.h"
#include "Utils.h"
#include "RenderPipeline.h"
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Renderer
{
	bool VulkanRenderer::Init(GLFWwindow* window)
	{
		PROFILE_FUNCTION();
		this->window = window;

		try
		{
			CreateInstance();
			CreateValidationDebugMessenger();
			CreateSurface();
			GetPhysicalDevice();
			CreateLogicalDevice();
			CreateSwapChain();
			CreateRenderPass();
			CreateColorBufferImage();
			CreateDepthBufferImage();
			CreateRenderPipeline();
			CreateFrameBuffers();
			CreateCommandPool();
			CreateCommandBuffers();
			CreateTextureSampler();
			CreateSynchronization();

			// Default texture, Will be assigned if no texture can be found on 3D model file
			CreateTexture("testTexture.jpg");

			renderPipelinePtr->SetPerspectiveProjectionMatrix(glm::radians(60.0f), (float)swapChainExtent.width / swapChainExtent.height, 0.1f, 1000.0f);
			renderPipelinePtr->SetViewMatrixFromLookAt(glm::vec3(0.0f, 0.0f, 200.0f), glm::vec3(0.0f), GLOBAL_UP);
			renderPipelinePtr->SetModelMatrix(glm::mat4(1.0f));

		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "\nVulkan Init error : " << e.what();
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	void VulkanRenderer::Update(int32_t modelId, const glm::mat4& modelMat)
	{
		PROFILE_FUNCTION();

		if (modelId >= 0 && modelId < modelList.size())
		{
			modelList[modelId].SetModelMatrix(modelMat);
		}
		else
		{
			throw std::runtime_error("Failed to update model, Invalid index");
		}
	}

	void VulkanRenderer::Draw()
	{
		PROFILE_FUNCTION();

		uint32_t imageIndex = 0;
		{
			PROFILE_SCOPE("Wait, Reset Fences & Accquire Image");

			vkWaitForFences(deviceHandle.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
			vkResetFences(deviceHandle.logicalDevice, 1, &drawFences[currentFrame]);

			vkAcquireNextImageKHR(deviceHandle.logicalDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);
		}

		RecordCommands(imageIndex);

		renderPipelinePtr->UpdateUniformBuffers(imageIndex, modelList);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinished[currentFrame];

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.pWaitDstStageMask = waitStages;

		{
			PROFILE_SCOPE("Queue Submit & Present");

			VkResult vkResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to submit command buffer to queue");
			}

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &renderFinished[currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapChain;
			presentInfo.pImageIndices = &imageIndex;

			vkResult = vkQueuePresentKHR(presentationQueue, &presentInfo);

			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to present to queue");
			}
		}

		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}

	void VulkanRenderer::CleanUp()
	{
		PROFILE_FUNCTION();
		vkDeviceWaitIdle(deviceHandle.logicalDevice);

		for (size_t i = 0; i < modelList.size(); i++)
		{
			modelList[i].DestroyModel();
		}

		vkDestroySampler(deviceHandle.logicalDevice, textureSampler, nullptr);

		for (size_t i = 0; i < textureHandles.size(); i++)
		{
			vkDestroyImageView(deviceHandle.logicalDevice, textureImgViews[i], nullptr);
			vkDestroyImage(deviceHandle.logicalDevice, textureHandles[i].image, nullptr);
			vkFreeMemory(deviceHandle.logicalDevice, textureHandles[i].memory, nullptr);
		}

		for (size_t i = 0; i < depthBufferImage.size(); i++)
		{
			vkDestroyImageView(deviceHandle.logicalDevice, depthBufferImageView[i], nullptr);
			vkDestroyImage(deviceHandle.logicalDevice, depthBufferImage[i], nullptr);
			vkFreeMemory(deviceHandle.logicalDevice, depthBufferImageMemory[i], nullptr);
		}

		for (size_t i = 0; i < colourBufferImage.size(); i++)
		{
			vkDestroyImageView(deviceHandle.logicalDevice, colourBufferImageView[i], nullptr);
			vkDestroyImage(deviceHandle.logicalDevice, colourBufferImage[i], nullptr);
			vkFreeMemory(deviceHandle.logicalDevice, colourBufferImageMemory[i], nullptr);
		}

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(deviceHandle.logicalDevice, renderFinished[i], nullptr);
			vkDestroySemaphore(deviceHandle.logicalDevice, imageAvailable[i], nullptr);
			vkDestroyFence(deviceHandle.logicalDevice, drawFences[i], nullptr);
		}

		vkDestroyCommandPool(deviceHandle.logicalDevice, gfxCommandPool, nullptr);

		for (auto frameBuffer : swapchainFrameBuffers)
		{
			vkDestroyFramebuffer(deviceHandle.logicalDevice, frameBuffer, nullptr);
		}

		for (const auto& image : swapChainImages)
		{
			vkDestroyImageView(deviceHandle.logicalDevice, image.imageView, nullptr);
		}

		vkDestroyRenderPass(deviceHandle.logicalDevice, renderPass, nullptr);

		if (renderPipelinePtr != nullptr)
		{
			delete renderPipelinePtr;
			renderPipelinePtr = nullptr;
		}

		vkDestroySwapchainKHR(deviceHandle.logicalDevice, swapChain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(deviceHandle.logicalDevice, nullptr);
		DestroyValidationDebugMessenger();
		vkDestroyInstance(instance, nullptr);
	}

	void VulkanRenderer::CreateInstance()
	{
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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

			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(deviceHandle.physicalDevice, &deviceProps);

			minUniformBufferOffset = deviceProps.limits.minUniformBufferOffsetAlignment;
		}
		else
		{
			throw std::runtime_error("Could not find vulkan compatible GPUs");
		}
	}

	void VulkanRenderer::CreateLogicalDevice()
	{
		PROFILE_FUNCTION();

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
		deviceFeatures.depthClamp = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;
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
		PROFILE_FUNCTION();

		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface");
		}
	}

	void VulkanRenderer::CreateSwapChain()
	{
		PROFILE_FUNCTION();

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

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(deviceHandle.logicalDevice, swapChain, &swapChainImageCount, nullptr);

		std::vector<VkImage> images(swapChainImageCount);
		vkGetSwapchainImagesKHR(deviceHandle.logicalDevice, swapChain, &swapChainImageCount, images.data());

		for (VkImage image : images)
		{
			SwapChainImage swapChainImage = {};
			swapChainImage.image = image;

			CreateImageViewInfo createImageViewInfo{};
			createImageViewInfo.image = image;
			createImageViewInfo.format = swapChainImageFormat;
			createImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageViewInfo.mipmapCount = 1;

			swapChainImage.imageView = CreateImageView(createImageViewInfo);

			swapChainImages.push_back(swapChainImage);
		}
	}

	void VulkanRenderer::CreateRenderPass()
	{
		PROFILE_FUNCTION();

		// Attachments
		// Subpass 1 attachments - Input Attachments

		std::array<VkSubpassDescription, 2> subpasses{};

		// Depth Attachment
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = GetSuitableFormat(
			{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Colour attachment
		VkAttachmentDescription colourAttachment = {};
		colourAttachment.format = GetSuitableFormat(
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Colour Attachment reference
		VkAttachmentReference colourAttachmentReference = {};
		colourAttachmentReference.attachment = 1;
		colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Depth Attachment reference
		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 2;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Set up Subpass 1
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = 1;
		subpasses[0].pColorAttachments = &colourAttachmentReference;
		subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

		// Subpass 2 attachments - Input Attachments

		// Swapchain Colour attachment
		VkAttachmentDescription swapchainColourAttachment{};
		swapchainColourAttachment.format = swapChainImageFormat;
		swapchainColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		swapchainColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		swapchainColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		swapchainColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		swapchainColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		swapchainColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Colour Attachment reference
		VkAttachmentReference swapchainColourAttachmentReference = {};
		swapchainColourAttachmentReference.attachment = 0;
		swapchainColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// References to attachments that subpass will take input from
		std::array<VkAttachmentReference, 2> inputReferences;
		inputReferences[0].attachment = 1;
		inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputReferences[1].attachment = 2;
		inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Set up Subpass 2
		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &swapchainColourAttachmentReference;
		subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
		subpasses[1].pInputAttachments = inputReferences.data();

		std::array<VkSubpassDependency, 3> subpassDependencies{};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;

		// Subpass 1 layout (colour/depth) to subpass 2 layout (shader read)
		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstSubpass = 1;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = 0;

		subpassDependencies[2].srcSubpass = 0;
		subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[2].dependencyFlags = 0;

		std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColourAttachment, colourAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
		renderPassCreateInfo.pAttachments = renderPassAttachments.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassCreateInfo.pSubpasses = subpasses.data();
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassCreateInfo.pDependencies = subpassDependencies.data();

		VkResult vkResult = vkCreateRenderPass(deviceHandle.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass");
		}

	}

	void VulkanRenderer::CreateRenderPipeline()
	{
		PROFILE_FUNCTION();

		using namespace Utilities;

		auto vertCode = Utils::ReadFile(COMPILED_SHADER_PATH + std::string("simple_shader.vert") + COMPILED_SHADER_SUFFIX);
		auto fragCode = Utils::ReadFile(COMPILED_SHADER_PATH + std::string("simple_shader.frag") + COMPILED_SHADER_SUFFIX);

		auto secondPassVertCode = Utils::ReadFile(COMPILED_SHADER_PATH + std::string("second_subpass.vert") + COMPILED_SHADER_SUFFIX);
		auto secondPassFragCode = Utils::ReadFile(COMPILED_SHADER_PATH + std::string("second_subpass.frag") + COMPILED_SHADER_SUFFIX);

		VkShaderModule vertexShaderModule = Utils::CreateShaderModule(deviceHandle.logicalDevice, vertCode);
		VkShaderModule fragmentShaderModule = Utils::CreateShaderModule(deviceHandle.logicalDevice, fragCode);

		VkShaderModule secondPassVertexShaderModule = Utils::CreateShaderModule(deviceHandle.logicalDevice, secondPassVertCode);
		VkShaderModule secondPassfragmentShaderModule = Utils::CreateShaderModule(deviceHandle.logicalDevice, secondPassFragCode);

		renderPipelinePtr = new RenderPipeline();

		RenderPipeline::RenderPipelineCreateInfo pipelineCreateInfo = {};
		RenderPipeline::ShaderModuleSet firstPassShaderModuleSet{ vertexShaderModule, fragmentShaderModule };
		RenderPipeline::ShaderModuleSet secondPassShaderModuleSet{ secondPassVertexShaderModule, secondPassfragmentShaderModule };

		pipelineCreateInfo.firstPassShaderModule = firstPassShaderModuleSet;
		pipelineCreateInfo.secondPassShaderModule = secondPassShaderModuleSet;

		pipelineCreateInfo.extent = swapChainExtent;
		pipelineCreateInfo.device = deviceHandle;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.swapchainImageCount = swapChainImages.size();
		pipelineCreateInfo.minUniformBufferOffset = minUniformBufferOffset;
		pipelineCreateInfo.colourBufferImageViewPtr = &colourBufferImageView;
		pipelineCreateInfo.depthBufferImageViewPtr = &depthBufferImageView;

		renderPipelinePtr->Init(pipelineCreateInfo);

		vkDestroyShaderModule(deviceHandle.logicalDevice, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(deviceHandle.logicalDevice, vertexShaderModule, nullptr);

		vkDestroyShaderModule(deviceHandle.logicalDevice, secondPassVertexShaderModule, nullptr);
		vkDestroyShaderModule(deviceHandle.logicalDevice, secondPassfragmentShaderModule, nullptr);
	}

	void VulkanRenderer::CreateColorBufferImage()
	{
		PROFILE_FUNCTION();

		colourBufferImage.resize(swapChainImages.size());
		colourBufferImageMemory.resize(swapChainImages.size());
		colourBufferImageView.resize(swapChainImages.size());

		VkFormat colourFormat = GetSuitableFormat(
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		CreateImageInfo imageCreateInfo = {};
		imageCreateInfo.format = colourFormat;
		imageCreateInfo.width = swapChainExtent.width;
		imageCreateInfo.height = swapChainExtent.height;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.useFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		imageCreateInfo.propFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			colourBufferImage[i] = CreateImage(imageCreateInfo, &colourBufferImageMemory[i]);

			CreateImageViewInfo createImageViewInfo{};
			createImageViewInfo.image = colourBufferImage[i];
			createImageViewInfo.format = colourFormat;
			createImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageViewInfo.mipmapCount = 1;

			colourBufferImageView[i] = CreateImageView(createImageViewInfo);
		}
	}

	void VulkanRenderer::CreateDepthBufferImage()
	{
		PROFILE_FUNCTION();

		depthBufferImage.resize(swapChainImages.size());
		depthBufferImageMemory.resize(swapChainImages.size());
		depthBufferImageView.resize(swapChainImages.size());

		VkFormat depthFormat = GetSuitableFormat(
			{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		CreateImageInfo imageCreateInfo = {};
		imageCreateInfo.format = depthFormat;
		imageCreateInfo.width = swapChainExtent.width;
		imageCreateInfo.height = swapChainExtent.height;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.useFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		imageCreateInfo.propFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			depthBufferImage[i] = CreateImage(imageCreateInfo, &depthBufferImageMemory[i]);

			CreateImageViewInfo createImageViewInfo{};
			createImageViewInfo.image = depthBufferImage[i];
			createImageViewInfo.format = depthFormat;
			createImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
			createImageViewInfo.mipmapCount = 1;

			depthBufferImageView[i] = CreateImageView(createImageViewInfo);
		}
	}

	void VulkanRenderer::CreateFrameBuffers()
	{
		PROFILE_FUNCTION();

		swapchainFrameBuffers.resize(swapChainImages.size());

		for (size_t i = 0; i < swapchainFrameBuffers.size(); i++)
		{
			std::array<VkImageView, 3> attachments = { swapChainImages[i].imageView, colourBufferImageView[i], depthBufferImageView[i] };

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCreateInfo.pAttachments = attachments.data();
			frameBufferCreateInfo.width = swapChainExtent.width;
			frameBufferCreateInfo.height = swapChainExtent.height;
			frameBufferCreateInfo.layers = 1;

			VkResult vkResult = vkCreateFramebuffer(deviceHandle.logicalDevice, &frameBufferCreateInfo, nullptr, &swapchainFrameBuffers[i]);

			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create a frame buffer");
			}
		}
	}

	void VulkanRenderer::CreateCommandPool()
	{
		PROFILE_FUNCTION();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		QueueFamilyIndices queueIndices = GetQueueFamilyIndices(deviceHandle.physicalDevice);
		poolInfo.queueFamilyIndex = queueIndices.graphicsFamily;

		VkResult vkResult = vkCreateCommandPool(deviceHandle.logicalDevice, &poolInfo, nullptr, &gfxCommandPool);

		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}
	}

	void VulkanRenderer::CreateCommandBuffers()
	{
		PROFILE_FUNCTION();

		commandBuffers.resize(swapchainFrameBuffers.size());
		VkCommandBufferAllocateInfo allocateInfo = {};

		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		allocateInfo.commandPool = gfxCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkResult vkResult = vkAllocateCommandBuffers(deviceHandle.logicalDevice, &allocateInfo, commandBuffers.data());

		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffer");
		}
	}

	void VulkanRenderer::CreateSynchronization()
	{
		PROFILE_FUNCTION();

		imageAvailable.resize(MAX_FRAME_DRAWS);
		renderFinished.resize(MAX_FRAME_DRAWS);
		drawFences.resize(MAX_FRAME_DRAWS);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VkResult vkResult1 = vkCreateSemaphore(deviceHandle.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]);
			VkResult vkResult2 = vkCreateSemaphore(deviceHandle.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]);
			VkResult vkResult3 = vkCreateFence(deviceHandle.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]);

			if (vkResult1 != VK_SUCCESS || vkResult2 != VK_SUCCESS || vkResult3 != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create semaphore");
			}
		}
	}

	void VulkanRenderer::CreateTextureSampler()
	{
		PROFILE_FUNCTION();

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16.0f;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 8.0f;

		VkResult vkResult = vkCreateSampler(deviceHandle.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);

		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture sampler!");
		}
	}

	int32_t VulkanRenderer::CreateTexture(const std::string& fileName, bool useMipmaps /*=false*/)
	{
		PROFILE_FUNCTION();

		uint32_t mipmapCount = 1;
		int32_t textureImgLoc = CreateTextureImage(fileName, useMipmaps ? &mipmapCount : nullptr);

		CreateImageViewInfo createImageViewInfo{};
		createImageViewInfo.image = textureHandles[textureImgLoc].image;
		createImageViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		createImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageViewInfo.mipmapCount = mipmapCount;

		VkImageView imgView = CreateImageView(createImageViewInfo);
		textureImgViews.push_back(imgView);

		int32_t descLoc = renderPipelinePtr->CreateTextureDescriptor(imgView, textureSampler);

		return descLoc;
	}

	int32_t VulkanRenderer::CreateTextureImage(const std::string& fileName, uint32_t* mipmapCount /*=nullptr*/)
	{
		PROFILE_FUNCTION();

		TextureInfo texInfo;
		stbi_uc* imageData = Utils::LoadTextureFile(fileName, texInfo);

		VkBuffer imageStagingBuffer = nullptr;
		VkDeviceMemory imageStagingBufferMemory = nullptr;

		CreateBufferInfo bufferInfo{};
		bufferInfo.physicalDevice = deviceHandle.physicalDevice;
		bufferInfo.device = deviceHandle.logicalDevice;
		bufferInfo.bufferSize = texInfo.imageSize;
		bufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.memoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bufferInfo.buffer = &imageStagingBuffer;
		bufferInfo.bufferMemory = &imageStagingBufferMemory;

		Utils::CreateBuffer(bufferInfo);

		void* data = nullptr;
		vkMapMemory(deviceHandle.logicalDevice, imageStagingBufferMemory, 0, texInfo.imageSize, 0, &data);
		memcpy(data, imageData, static_cast<size_t>(texInfo.imageSize));
		vkUnmapMemory(deviceHandle.logicalDevice, imageStagingBufferMemory);

		stbi_image_free(imageData);

		// Create image to hold final texture
		VkImage texImage;
		VkDeviceMemory texImageMemory;

		if (mipmapCount != nullptr)
		{
			*mipmapCount = static_cast<uint32_t>(std::floor(std::log2(std::max(texInfo.width, texInfo.height)))) + 1;
		}

		CreateImageInfo createImageInfo{};
		createImageInfo.width = texInfo.width;
		createImageInfo.height = texInfo.height;
		createImageInfo.mipmapCount = (mipmapCount == nullptr) ? 1 : *mipmapCount;
		createImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		createImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createImageInfo.useFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		createImageInfo.propFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		texImage = CreateImage(createImageInfo, &texImageMemory);

		// Transition image to be destination for copy operation

		TransitionImageLayoutInfo transitionInfo{};
		transitionInfo.device = deviceHandle.logicalDevice;
		transitionInfo.cmdPool = gfxCommandPool;
		transitionInfo.queue = graphicsQueue;
		transitionInfo.image = texImage;
		transitionInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		transitionInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		transitionInfo.mipmapCount = createImageInfo.mipmapCount;

		Utils::TransitionImageLayout(transitionInfo);

		// Copy data to image

		CopyImageBufferInfo cpyImgBufInfo{};
		cpyImgBufInfo.device = deviceHandle.logicalDevice;
		cpyImgBufInfo.width = texInfo.width;
		cpyImgBufInfo.height = texInfo.height;
		cpyImgBufInfo.transferQueue = graphicsQueue;
		cpyImgBufInfo.transCommandPool = gfxCommandPool;
		cpyImgBufInfo.srcBuffer = imageStagingBuffer;
		cpyImgBufInfo.dstImage = texImage;

		Utils::CopyImageBuffer(cpyImgBufInfo);

		textureHandles.push_back({ texImage, texImageMemory });

		vkDestroyBuffer(deviceHandle.logicalDevice, imageStagingBuffer, nullptr);
		vkFreeMemory(deviceHandle.logicalDevice, imageStagingBufferMemory, nullptr);

		CreateMipmapInfo createMipmapInfo{};
		createMipmapInfo.image = texImage;
		createMipmapInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		createMipmapInfo.texWidth = texInfo.width;
		createMipmapInfo.texHeight = texInfo.height;
		createMipmapInfo.mipLevels = createImageInfo.mipmapCount;

		GenerateMipmaps(createMipmapInfo);

		return static_cast<int32_t>(textureHandles.size()) - 1;
	}

	void VulkanRenderer::GenerateMipmaps(const CreateMipmapInfo& createMipmapInfo)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(deviceHandle.physicalDevice, createMipmapInfo.imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = Utils::BeginCmdBuffer(deviceHandle.logicalDevice, gfxCommandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = createMipmapInfo.image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = createMipmapInfo.texWidth;
		int32_t mipHeight = createMipmapInfo.texHeight;

		for (uint32_t i = 1; i < createMipmapInfo.mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				createMipmapInfo.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				createMipmapInfo.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = createMipmapInfo.mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		Utils::EndAndSubmitCmdBuffer(deviceHandle.logicalDevice, gfxCommandPool, graphicsQueue, commandBuffer);
	}

	int32_t VulkanRenderer::CreateModel(const std::string& fileName, float scaleFactor /*= 1.0f*/)
	{
		PROFILE_FUNCTION();

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(MODELS_PATH + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

		if (scene == nullptr)
		{
			throw std::runtime_error("Failed to load model : " + fileName);
		}

		std::vector<std::string> textureNames = Model::LoadMaterials(scene);
		std::vector<int> matToTex(textureNames.size());

		for (size_t i = 0; i < textureNames.size(); i++)
		{
			if (textureNames[i].empty())
			{
				matToTex[i] = 0;
			}
			else
			{
				matToTex[i] = CreateTexture(textureNames[i], true);
			}
		}

		std::vector<Mesh> modelMeshes = Model::LoadNode(deviceHandle.physicalDevice, deviceHandle.logicalDevice,
			graphicsQueue, gfxCommandPool, scene->mRootNode, scene, matToTex, scaleFactor);

		Model model = Model(modelMeshes);
		modelList.push_back(model);

		return modelList.size() - 1;
	}

	void VulkanRenderer::RecordCommands(uint32_t currentImageIndex)
	{
		PROFILE_FUNCTION();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkRenderPassBeginInfo renderpassBeginInfo = {};
		renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassBeginInfo.renderPass = renderPass;
		renderpassBeginInfo.renderArea.offset = { 0, 0 };
		renderpassBeginInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 3> clearValues = {};
		clearValues[0].color = { 1.0f, 0.0f, 1.0f, 1.0f };
		clearValues[1].color = { 0.3f, 0.5f, 0.6f, 1.0f };
		clearValues[2].depthStencil.depth = 1.0f;
		renderpassBeginInfo.pClearValues = clearValues.data();
		renderpassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

		renderpassBeginInfo.framebuffer = swapchainFrameBuffers[currentImageIndex];

		VkResult vkResult = vkBeginCommandBuffer(commandBuffers[currentImageIndex], &beginInfo);
		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer");
		}

		vkCmdBeginRenderPass(commandBuffers[currentImageIndex], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind pipeline to used in render pass

		vkCmdBindPipeline(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipelinePtr->GetPipeline());

		for (size_t j = 0; j < modelList.size(); j++)
		{
			Model thisModel = modelList[j];
			vkCmdPushConstants(commandBuffers[currentImageIndex], renderPipelinePtr->GetPipelineLayout(),
				VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &thisModel.GetModelMatrix());

			for (size_t k = 0; k < thisModel.GetMeshCount(); k++)
			{
				VkBuffer vertexBuffers[] = { thisModel.GetMesh(k)->GetVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[currentImageIndex], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffers[currentImageIndex], thisModel.GetMesh(k)->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				//Dynamic offset amount
				uint32_t dynamicOffset = renderPipelinePtr->GetModelUniformAlignment() * static_cast<uint32_t>(j);

				std::array<VkDescriptorSet, 2> descSetGroup = {
					renderPipelinePtr->GetDescriptorSet(currentImageIndex),
					renderPipelinePtr->GetSamplerDescriptorSet(thisModel.GetMesh(k)->GetTexId()) };

				vkCmdBindDescriptorSets(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
					renderPipelinePtr->GetPipelineLayout(), 0, static_cast<uint32_t>(descSetGroup.size()), descSetGroup.data(), 1, &dynamicOffset);

				vkCmdDrawIndexed(commandBuffers[currentImageIndex], static_cast<uint32_t>(thisModel.GetMesh(k)->GetIndexCount()), 1, 0, 0, 0);
			}
		}

		// Start second sub pass

		vkCmdNextSubpass(commandBuffers[currentImageIndex], VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipelinePtr->GetSecondPipeline());
		vkCmdBindDescriptorSets(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipelinePtr->GetSecondPipelineLayout(),
			0, 1, &renderPipelinePtr->GetInputDescriptorSet(currentImageIndex), 0, nullptr);

		vkCmdDraw(commandBuffers[currentImageIndex], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers[currentImageIndex]);

		vkResult = vkEndCommandBuffer(commandBuffers[currentImageIndex]);
		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to end command buffer");
		}

	}

	bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions) const
	{
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

		VkPhysicalDeviceFeatures deviceFeatures = {};
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = GetQueueFamilyIndices(device);
		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainValid = false;
		if (extensionsSupported)
		{
			SwapChainInfo swapChainInfo = GetSwapChainDetails(device);
			swapChainValid = !swapChainInfo.presentationModes.empty() && !swapChainInfo.surfaceFormats.empty();
		}

		return indices.IsValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
	}

	bool VulkanRenderer::CheckValidationLayerSupport(std::vector<const char*>* validationLayers) const
	{
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(index), surface, &presentationSupport);

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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
		PROFILE_FUNCTION();

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

	VkFormat VulkanRenderer::GetSuitableFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const
	{
		PROFILE_FUNCTION();

		for (VkFormat format : formats)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(deviceHandle.physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find a matching format!");
	}

	VkImage VulkanRenderer::CreateImage(const CreateImageInfo& createImageInfo, VkDeviceMemory* imageMemory) const
	{
		PROFILE_FUNCTION();

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = createImageInfo.width;
		imageCreateInfo.extent.height = createImageInfo.height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = createImageInfo.mipmapCount;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = createImageInfo.format;
		imageCreateInfo.tiling = createImageInfo.tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = createImageInfo.useFlags;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkImage image;

		VkResult vkResult = vkCreateImage(deviceHandle.logicalDevice, &imageCreateInfo, nullptr, &image);
		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image ");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(deviceHandle.logicalDevice, image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = Utils::FindMemoryTypeIndex(deviceHandle.physicalDevice, memoryRequirements.memoryTypeBits, createImageInfo.propFlags);

		vkResult = vkAllocateMemory(deviceHandle.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for image");
		}

		vkBindImageMemory(deviceHandle.logicalDevice, image, *imageMemory, 0);

		return image;
	}

	VkImageView VulkanRenderer::CreateImageView(const CreateImageViewInfo& createImageViewInfo)
	{
		PROFILE_FUNCTION();

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = createImageViewInfo.image;
		viewCreateInfo.format = createImageViewInfo.format;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = createImageViewInfo.aspectFlags;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = createImageViewInfo.mipmapCount;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(deviceHandle.logicalDevice, &viewCreateInfo, nullptr, &imageView);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view");
		}

		return imageView;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:/*std::cerr << " [Verbose]:"; break*/ return VK_FALSE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:std::cerr << "\nValidation [Info]:"; break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:std::cerr << "\nValidation [Warning]:"; break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:	std::cerr << "\nValidation [Error]:"; break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:break;
		}

		std::cerr << pCallbackData->pMessage;
		return VK_FALSE;
	}
}