#pragma once
#include <stb/stb_image.h>
#include "ConstantsAndDefines.h"

#define PROFILE_SCOPE(name) BenchmarkTimer timer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)

#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>

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

	struct DeviceHandle
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
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
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct UboViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	struct CreateImageInfo
	{
		uint32_t width;
		uint32_t height;
		VkFormat format;
		VkImageTiling tiling;
		VkImageUsageFlags useFlags;
		VkMemoryPropertyFlags propFlags;
	};

	struct CreateBufferInfo
	{
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkDeviceSize bufferSize;
		VkBufferUsageFlags bufferUsageFlags;
		VkMemoryPropertyFlags memoryPropFlags;
		VkBuffer* buffer;
		VkDeviceMemory* bufferMemory;
	};

	struct CopyBufferInfo
	{
		VkDevice device;
		VkQueue transferQueue;
		VkCommandPool transCommandPool;
		VkBuffer srcBuffer;
		VkBuffer dstBuffer;
		VkDeviceSize bufferSize;
	};

	struct CopyImageBufferInfo
	{
		VkDevice device;
		VkQueue transferQueue;
		VkCommandPool transCommandPool;
		VkBuffer srcBuffer;
		VkImage dstImage;
		uint32_t width;
		uint32_t height;
	};

	struct TransitionImageLayoutInfo
	{
		VkDevice device;
		VkQueue queue;
		VkCommandPool cmdPool;
		VkImage image;
		VkImageLayout oldLayout;
		VkImageLayout newLayout;
	};

	struct TextureInfo
	{
		int32_t width = 0;
		int32_t height = 0;
		int32_t channelCount = 0;
		VkDeviceSize imageSize;
	};

	struct TextureHandle
	{
		VkImage image;
		VkDeviceMemory memory;
	};

	struct ProfileResult
	{
		std::string Name;
		long long Start, End;
		size_t ThreadID;
	};

	struct BenchmarkSession
	{
		std::string Name;
	};

	struct Benchmark
	{
	public:
		Benchmark() : currentSession(nullptr), profileCount(0) {}

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			outputStream.open(filepath);
			WriteHeader();
			currentSession = new BenchmarkSession{ name };
		}

		void EndSession()
		{
			WriteFooter();
			outputStream.close();
			delete currentSession;
			currentSession = nullptr;
			profileCount = 0;
		}

		void WriteProfile(const ProfileResult& result)
		{
			if (profileCount++ > 0)
				outputStream << ",";

			std::string name = result.Name;
			std::replace(name.begin(), name.end(), '"', '\'');

			const std::string toEraseStr("__cdecl");
			const size_t pos = name.find(toEraseStr);
			if (pos != std::string::npos)
			{
				name.erase(pos, toEraseStr.length());
			}

			outputStream << "{";
			outputStream << "\"cat\":\"function\",";
			outputStream << "\"dur\":" << (result.End - result.Start) << ',';
			outputStream << "\"name\":\"" << name << "\",";
			outputStream << "\"ph\":\"X\",";
			outputStream << "\"pid\":0,";
			outputStream << "\"tid\":" << result.ThreadID << ",";
			outputStream << "\"ts\":" << result.Start;
			outputStream << "}";

			outputStream.flush();
		}

		void WriteHeader()
		{
			outputStream << "{\"otherData\": {},\"traceEvents\":[";
			outputStream.flush();
		}

		void WriteFooter()
		{
			outputStream << "]}";
			outputStream.flush();
		}

		static Benchmark& Get()
		{
			static Benchmark instance;
			return instance;
		}

	private:
		BenchmarkSession* currentSession;
		std::ofstream outputStream;
		int profileCount;
	};

	struct BenchmarkTimer
	{
	public:
		BenchmarkTimer(const char* name) : name(name), stopped(false)
		{
			startTimepoint = std::chrono::high_resolution_clock::now();
		}

		~BenchmarkTimer()
		{
			if (!stopped)
			{
				Stop();
			}
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::high_resolution_clock::now();

			long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

			size_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
			Benchmark::Get().WriteProfile({ name, start, end, threadID });

			stopped = true;
		}

	private:
		const char* name;
		std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint;
		bool stopped;
	};

	class Utils
	{
	public:

		static std::vector<char> ReadFile(const std::string& filePath)
		{
			PROFILE_FUNCTION();

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
			PROFILE_FUNCTION();

			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

			for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
			{
				if ((allowedTypes & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memPropFlags) == memPropFlags)
				{
					return i;
				}
			}

			return NULL;
		}

		static void CreateBuffer(const CreateBufferInfo& bufferInfo)
		{
			PROFILE_FUNCTION();

			VkBufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.size = bufferInfo.bufferSize;
			createInfo.usage = bufferInfo.bufferUsageFlags;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult vkResult = vkCreateBuffer(bufferInfo.device, &createInfo, nullptr, bufferInfo.buffer);

			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create vertex buffer");
			}

			VkMemoryRequirements memRequirements = {};
			vkGetBufferMemoryRequirements(bufferInfo.device, *bufferInfo.buffer, &memRequirements);

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memRequirements.size;
			memAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(bufferInfo.physicalDevice, memRequirements.memoryTypeBits,
				bufferInfo.memoryPropFlags);

			vkResult = vkAllocateMemory(bufferInfo.device, &memAllocInfo, nullptr, bufferInfo.bufferMemory);
			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to allocate vertex buffer memory");
			}

			vkBindBufferMemory(bufferInfo.device, *bufferInfo.buffer, *bufferInfo.bufferMemory, 0);
		}

		static void CopyBuffer(const CopyBufferInfo& copyBufferInfo)
		{
			PROFILE_FUNCTION();

			VkCommandBuffer transferCommandBuffer = BeginCmdBuffer(copyBufferInfo.device, copyBufferInfo.transCommandPool);

			VkBufferCopy bufferCopyRegion = {};
			bufferCopyRegion.srcOffset = 0;
			bufferCopyRegion.dstOffset = 0;
			bufferCopyRegion.size = copyBufferInfo.bufferSize;

			vkCmdCopyBuffer(transferCommandBuffer, copyBufferInfo.srcBuffer, copyBufferInfo.dstBuffer, 1, &bufferCopyRegion);

			EndAndSubmitCmdBuffer(copyBufferInfo.device, copyBufferInfo.transCommandPool, copyBufferInfo.transferQueue, transferCommandBuffer);
		}

		static void CopyImageBuffer(const CopyImageBufferInfo& copyImgBufferInfo)
		{
			PROFILE_FUNCTION();

			VkCommandBuffer transferCommandBuffer = BeginCmdBuffer(copyImgBufferInfo.device, copyImgBufferInfo.transCommandPool);

			VkBufferImageCopy imageRegion = {};
			imageRegion.bufferOffset = 0;
			imageRegion.bufferRowLength = 0;
			imageRegion.bufferImageHeight = 0;
			imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageRegion.imageSubresource.mipLevel = 0;
			imageRegion.imageSubresource.baseArrayLayer = 0;
			imageRegion.imageSubresource.layerCount = 1;
			imageRegion.imageOffset = { 0, 0, 0 };
			imageRegion.imageExtent = { copyImgBufferInfo.width, copyImgBufferInfo.height, 1 };

			vkCmdCopyBufferToImage(transferCommandBuffer, copyImgBufferInfo.srcBuffer, copyImgBufferInfo.dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

			EndAndSubmitCmdBuffer(copyImgBufferInfo.device, copyImgBufferInfo.transCommandPool, copyImgBufferInfo.transferQueue, transferCommandBuffer);
		}

		static VkCommandBuffer BeginCmdBuffer(VkDevice device, VkCommandPool cmdPool)
		{
			PROFILE_FUNCTION();

			VkCommandBuffer cmdBuffer;

			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandPool = cmdPool;
			allocateInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(device, &allocateInfo, &cmdBuffer);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(cmdBuffer, &beginInfo);

			return cmdBuffer;
		}

		static void EndAndSubmitCmdBuffer(VkDevice device, VkCommandPool cmdPool, VkQueue queue, VkCommandBuffer cmdBuffer)
		{
			PROFILE_FUNCTION();

			vkEndCommandBuffer(cmdBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);

			vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
		}

		static void TransitionImageLayout(const TransitionImageLayoutInfo& transitionImgLytInfo)
		{
			PROFILE_FUNCTION();

			VkCommandBuffer transferCommandBuffer = BeginCmdBuffer(transitionImgLytInfo.device, transitionImgLytInfo.cmdPool);

			VkImageMemoryBarrier imgMemoryBarrier = {};
			imgMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imgMemoryBarrier.oldLayout = transitionImgLytInfo.oldLayout;
			imgMemoryBarrier.newLayout = transitionImgLytInfo.newLayout;
			imgMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgMemoryBarrier.image = transitionImgLytInfo.image;
			imgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imgMemoryBarrier.subresourceRange.levelCount = 1;
			imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemoryBarrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;

			if (transitionImgLytInfo.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && transitionImgLytInfo.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				imgMemoryBarrier.srcAccessMask = 0;
				imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (transitionImgLytInfo.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && transitionImgLytInfo.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}

			vkCmdPipelineBarrier(transferCommandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imgMemoryBarrier);

			EndAndSubmitCmdBuffer(transitionImgLytInfo.device, transitionImgLytInfo.cmdPool, transitionImgLytInfo.queue, transferCommandBuffer);
		}

		static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
		{
			PROFILE_FUNCTION();

			VkShaderModuleCreateInfo shaderModuldeCreateInfo = {};
			shaderModuldeCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuldeCreateInfo.codeSize = code.size();
			shaderModuldeCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			VkResult vkResult = vkCreateShaderModule(device, &shaderModuldeCreateInfo, nullptr, &shaderModule);

			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader module");
			}

			return shaderModule;
		}

		static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
			VkResult result = vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);

			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create image view");
			}

			return imageView;
		}

		static VkImage CreateImage(DeviceHandle deviceHandle, const CreateImageInfo& createImageInfo, VkDeviceMemory* imageMemory)
		{
			PROFILE_FUNCTION();

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = createImageInfo.width;
			imageCreateInfo.extent.height = createImageInfo.height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
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

		static stbi_uc* LoadTextureFile(const std::string& fileName, TextureInfo& texInfo)
		{
			PROFILE_FUNCTION();

			const std::string fileLoc = TEXTURE_PATH + fileName;
			int width, height, channels;

			stbi_uc* image = stbi_load(fileLoc.c_str(), &width, &height, &channels, STBI_rgb_alpha);

			if (image == nullptr)
			{
				throw std::runtime_error("Failed to load texture : " + fileName);
			}

			texInfo.channelCount = channels;
			texInfo.height = height;
			texInfo.width = width;
			texInfo.imageSize = static_cast<VkDeviceSize>(width) * height * 4;

			return image;
		}
	};

}