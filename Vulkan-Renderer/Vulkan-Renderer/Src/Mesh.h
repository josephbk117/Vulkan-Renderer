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
	Mesh(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Vertex>* vertices);
	size_t GetVertexCount() const;
	VkBuffer GetVertexBuffer() const;
	void DestroyVertexBuffer();
	~Mesh();

private:
	size_t vertexCount = 0;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void CreateVertexBuffer(std::vector<Vertex>* vertices);
};

