#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Vertex>* vertices)
{
	vertexCount = vertices->size();
	this->physicalDevice = physicalDevice;
	this->device = device;
	CreateVertexBuffer(vertices);
}

size_t Mesh::GetVertexCount() const
{
	return vertexCount;
}

VkBuffer Mesh::GetVertexBuffer() const
{
	return vertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{

}

void Mesh::CreateVertexBuffer(std::vector<Vertex>* vertices)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = sizeof(Vertex) * vertices->size();
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult vkResult = vkCreateBuffer(device, &createInfo, nullptr, &vertexBuffer);

	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vertex buffer");
	}

	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = Utils::FindMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkResult = vkAllocateMemory(device, &memAllocInfo, nullptr, &vertexBufferMemory);
	if (vkResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory");
	}

	vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

	void* data;
	vkMapMemory(device, vertexBufferMemory, 0, createInfo.size, 0, &data);
	memcpy(data, vertices->data(), static_cast<size_t>(createInfo.size));
	vkUnmapMemory(device, vertexBufferMemory);
}
