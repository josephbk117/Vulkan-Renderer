#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utils.h"

using namespace Utilities;

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices);
	size_t GetVertexCount() const;
	size_t GetIndexCount() const;
	VkBuffer GetVertexBuffer() const;
	VkBuffer GetIndexBuffer() const;
	void DestroyBuffers();
	~Mesh();

private:
	size_t vertexCount = 0;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	size_t indexCount = 0;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

