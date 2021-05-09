#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utils.h"

using namespace Utilities;

struct UboModel
{
	glm::mat4 model;
};

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, uint32_t texId);
	size_t GetVertexCount() const;
	size_t GetIndexCount() const;
	VkBuffer GetVertexBuffer() const;
	VkBuffer GetIndexBuffer() const;
	void DestroyBuffers();
	void SetModel(const glm::mat4& newModel);
	UboModel GetModel() const;
	uint32_t GetTexId() const;
	~Mesh();

private:

	UboModel uboModel;

	uint32_t texId;

	size_t vertexCount = 0;
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;

	size_t indexCount = 0;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;

	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;

	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

