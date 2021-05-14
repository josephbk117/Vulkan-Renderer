#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <GLM/gtc/matrix_transform.hpp>
#include <vector>
#include <set>
#include "Utils.h"
#include "Mesh.h"
#include "Model.h"

using namespace Utilities;
namespace Renderer
{
	class RenderPipeline;

	class VulkanRenderer
	{
	public:
		bool Init(GLFWwindow* window);
		void Update();
		void Draw();
		void CleanUp();

	private:
		mutable DeviceHandle deviceHandle;

		GLFWwindow* window = nullptr;
		int currentFrame = 0;

		//Scene Objects
		std::vector<Mesh> meshList;

		VkInstance instance;
		VkQueue graphicsQueue;
		VkQueue presentationQueue;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		VkSampler textureSampler;

		mutable VkDeviceSize minUniformBufferOffset;

		VkRenderPass renderPass;
		RenderPipeline* renderPipelinePtr = nullptr;

		std::vector<SwapChainImage> swapChainImages;
		std::vector<VkFramebuffer> swapchainFrameBuffers;
		std::vector<VkCommandBuffer> commandBuffers;

		VkCommandPool gfxCommandPool;

		VkImage depthBufferImage;
		VkDeviceMemory depthBufferImageMemory;
		VkImageView depthBufferImageView;

		// Assets
		std::vector<TextureHandle> textureHandles;
		std::vector<VkImageView> textureImgViews;
		std::vector<Model> modelList;

		// Synchronization
		std::vector<VkSemaphore> imageAvailable;
		std::vector<VkSemaphore> renderFinished;
		std::vector<VkFence> drawFences;

		// Misc
		VkDebugUtilsMessengerEXT debugMessenger;


		void CreateInstance();
		void CreateValidationDebugMessenger();
		void DestroyValidationDebugMessenger();
		void GetPhysicalDevice()const;
		void CreateLogicalDevice();
		void CreateSurface();
		void CreateSwapChain();
		void CreateRenderPass();
		void CreateRenderPipeline();
		void CreateDepthBufferImage();
		void CreateFrameBuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSynchronization();
		void CreateTextureSampler();
		int32_t CreateTexture(const std::string& fileName);
		int32_t CreateTextureImage(const std::string& fileName);
		void CreateModel(const std::string& fileName, float scaleFactor = 1.0f);
		void RecordCommands(uint32_t currentImageIndex);
		bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice physDevice) const;
		bool CheckDeviceSuitable(VkPhysicalDevice device) const;
		bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers) const;
		QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device) const;
		SwapChainInfo GetSwapChainDetails(VkPhysicalDevice device)const;
		VkSurfaceFormatKHR GetSuitableSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) const;
		VkPresentModeKHR GetSuitablePresentationMode(const std::vector<VkPresentModeKHR>& presentationMode) const;
		VkExtent2D GetSuitableSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;
		VkFormat GetSuitableFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const;
		VkImage CreateImage(const CreateImageInfo& createImageInfo, VkDeviceMemory* imageMemory) const;
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkShaderModule CreateShaderModule(const std::vector<char>& code);

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	};
}