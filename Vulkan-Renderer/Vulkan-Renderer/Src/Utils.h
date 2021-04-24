#pragma once
#define GLFW_INCLUDE_VULKAN
#include <vector>
#include <string>
#include <fstream>
#include <GLM/glm.hpp>
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

			return NULL;
		}

		static void CreateBuffer(const CreateBufferInfo& bufferInfo)
		{
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
			VkCommandBuffer transferCommandBuffer;

			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandPool = copyBufferInfo.transCommandPool;
			allocateInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(copyBufferInfo.device, &allocateInfo, &transferCommandBuffer);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

			VkBufferCopy bufferCopyRegion = {};
			bufferCopyRegion.srcOffset = 0;
			bufferCopyRegion.dstOffset = 0;
			bufferCopyRegion.size = copyBufferInfo.bufferSize;

			vkCmdCopyBuffer(transferCommandBuffer, copyBufferInfo.srcBuffer, copyBufferInfo.dstBuffer, 1, &bufferCopyRegion);

			vkEndCommandBuffer(transferCommandBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCommandBuffer;

			vkQueueSubmit(copyBufferInfo.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(copyBufferInfo.transferQueue);

			vkFreeCommandBuffers(copyBufferInfo.device, copyBufferInfo.transCommandPool, 1, &transferCommandBuffer);
		}
	};

}