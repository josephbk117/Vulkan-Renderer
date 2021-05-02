#include "VulkanRenderer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <string>
#include "ConstantsAndDefines.h"
#include "Utils.h"
#include "RenderPipeline.h"
#include <array>

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
			CreateRenderPipeline();
			CreateFrameBuffers();
			CreateCommandPool();
			CreateCommandBuffers();

			renderPipelinePtr->SetPerspectiveProjectionMatrix(glm::radians(60.0f), (float)swapChainExtent.width / swapChainExtent.height, 0.1f, 100.0f);
			renderPipelinePtr->SetViewMatrixFromLookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f), GLOBAL_UP);
			renderPipelinePtr->SetModelMatrix(glm::mat4(1.0f));

			//Create meshes
			std::vector<Vertex> meshVertices =
			{ {{0.0f, -0.0f, 0.0}, {1.0, 0.0, 0.0}},
			  {{0.25, 0.5, 0.0}, {0,1,0}},
			  {{-0.25, 0.5, 0.0}, {0,0,1}} };

			std::vector<uint32_t> meshIndices = { 0, 1, 2 };
			meshList.emplace_back(deviceHandle.physicalDevice, deviceHandle.logicalDevice, graphicsQueue, gfxCommandPool, &meshVertices, &meshIndices);
			meshList.emplace_back(deviceHandle.physicalDevice, deviceHandle.logicalDevice, graphicsQueue, gfxCommandPool, &meshVertices, &meshIndices);

			meshList[0].SetModel(glm::mat4(1.0f));

			CreateSynchronization();
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "\nVulkan Init error : " << e.what();
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	void VulkanRenderer::Update()
	{
		PROFILE_FUNCTION();

		static float angle = 0.0f;
		static float lastTime = 0.0f;

		float deltaTime = 0.0f;

		float now = glfwGetTime();
		deltaTime = now - lastTime;

		angle += deltaTime;

		meshList[0].SetModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle * 100.0f), GLOBAL_FORWARD));
		meshList[1].SetModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle * -100.0f), GLOBAL_FORWARD));

		lastTime = now;
	}

	void VulkanRenderer::Draw()
	{
		PROFILE_FUNCTION();

		vkWaitForFences(deviceHandle.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(deviceHandle.logicalDevice, 1, &drawFences[currentFrame]);

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(deviceHandle.logicalDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

		RecordCommands(imageIndex);

		renderPipelinePtr->UpdateUniformBuffers(imageIndex, meshList);

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

		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}

	void VulkanRenderer::CleanUp()
	{
		PROFILE_FUNCTION();
		vkDeviceWaitIdle(deviceHandle.logicalDevice);

		for (size_t i = 0; i < meshList.size(); i++)
		{
			meshList[i].DestroyBuffers();
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

		for (auto image : swapChainImages)
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
			swapChainImage.imageView = CreateImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

			swapChainImages.push_back(swapChainImage);
		}
	}

	void VulkanRenderer::CreateRenderPass()
	{
		PROFILE_FUNCTION();

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;

		std::array<VkSubpassDependency, 2> subpassDependencies;
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
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

		VkShaderModule vertexShaderModule = CreateShaderModule(vertCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragCode);

		renderPipelinePtr = new RenderPipeline();

		RenderPipeline::RenderPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.vertexModule = vertexShaderModule;
		pipelineCreateInfo.fragmentModule = fragmentShaderModule;
		pipelineCreateInfo.extent = swapChainExtent;
		pipelineCreateInfo.device = deviceHandle;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.swapchainImageCount = swapChainImages.size();
		pipelineCreateInfo.minUniformBufferOffset = minUniformBufferOffset;

		renderPipelinePtr->Init(pipelineCreateInfo);

		vkDestroyShaderModule(deviceHandle.logicalDevice, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(deviceHandle.logicalDevice, vertexShaderModule, nullptr);
	}

	void VulkanRenderer::CreateFrameBuffers()
	{
		PROFILE_FUNCTION();

		swapchainFrameBuffers.resize(swapChainImages.size());

		for (size_t i = 0; i < swapchainFrameBuffers.size(); i++)
		{
			std::array<VkImageView, 1> attachments = { swapChainImages[i].imageView };

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

		VkClearValue clearValues[] = { {0.3f, 0.5f, 0.6f, 1.0f} };
		renderpassBeginInfo.pClearValues = clearValues;
		renderpassBeginInfo.clearValueCount = 1;

		renderpassBeginInfo.framebuffer = swapchainFrameBuffers[currentImageIndex];

		VkResult vkResult = vkBeginCommandBuffer(commandBuffers[currentImageIndex], &beginInfo);
		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer");
		}

		vkCmdBeginRenderPass(commandBuffers[currentImageIndex], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind pipeline to used in render pass

		vkCmdBindPipeline(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipelinePtr->GetPipeline());

		for (size_t j = 0; j < meshList.size(); j++)
		{

			VkBuffer vertexBuffers[] = { meshList[j].GetVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[currentImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[currentImageIndex], meshList[j].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			//Dynamic offset amount
			uint32_t dynamicOffset = renderPipelinePtr->GetModelUniformAlignment() * j;

			vkCmdBindDescriptorSets(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
				renderPipelinePtr->GetPipelineLayout(), 0, 1, &renderPipelinePtr->GetDescriptorSet(currentImageIndex), 1, &dynamicOffset);

			vkCmdDrawIndexed(commandBuffers[currentImageIndex], static_cast<uint32_t>(meshList[j].GetIndexCount()), 1, 0, 0, 0);
		}

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

	VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		PROFILE_FUNCTION();

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image;
		viewCreateInfo.format = format;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
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

	VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code)
	{
		PROFILE_FUNCTION();

		VkShaderModuleCreateInfo shaderModuldeCreateInfo = {};
		shaderModuldeCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuldeCreateInfo.codeSize = code.size();
		shaderModuldeCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		VkResult vkResult = vkCreateShaderModule(deviceHandle.logicalDevice, &shaderModuldeCreateInfo, nullptr, &shaderModule);

		if (vkResult != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
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