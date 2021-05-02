#include "Mesh.h"
#include "ConstantsAndDefines.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices)
{
	PROFILE_FUNCTION();

	vertexCount = vertices->size();
	indexCount = indices->size();
	this->physicalDevice = physicalDevice;
	this->device = device;
	CreateVertexBuffer(transferQueue, transferCommandPool, vertices);
	CreateIndexBuffer(transferQueue, transferCommandPool, indices);

	uboModel.model = glm::mat4(1.0f);
}

size_t Mesh::GetVertexCount() const
{
	return vertexCount;
}

size_t Mesh::GetIndexCount() const
{
	return indexCount;
}

VkBuffer Mesh::GetVertexBuffer() const
{
	return vertexBuffer;
}

VkBuffer Mesh::GetIndexBuffer() const
{
	return indexBuffer;
}

void Mesh::DestroyBuffers()
{
	PROFILE_FUNCTION();

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}

void Mesh::SetModel(const glm::mat4& newModel)
{
	uboModel.model = newModel;
}

UboModel Mesh::GetModel() const
{
	return uboModel;
}

Mesh::~Mesh()
{

}

void Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	PROFILE_FUNCTION();

	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBufferInfo bufferInfo;
	bufferInfo.physicalDevice = physicalDevice;
	bufferInfo.device = device;
	bufferInfo.bufferSize = bufferSize;
	bufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.memoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufferInfo.buffer = &stagingBuffer;
	bufferInfo.bufferMemory = &stagingBufferMemory;

	Utils::CreateBuffer(bufferInfo);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	CreateBufferInfo dstBufferInfo;
	dstBufferInfo.physicalDevice = physicalDevice;
	dstBufferInfo.device = device;
	dstBufferInfo.bufferSize = bufferSize;
	dstBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	dstBufferInfo.memoryPropFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	dstBufferInfo.buffer = &vertexBuffer;
	dstBufferInfo.bufferMemory = &vertexBufferMemory;

	Utils::CreateBuffer(dstBufferInfo);

	CopyBufferInfo copyBufferInfo;
	copyBufferInfo.device = device;
	copyBufferInfo.bufferSize = bufferSize;
	copyBufferInfo.transferQueue = transferQueue;
	copyBufferInfo.transCommandPool = transferCommandPool;
	copyBufferInfo.srcBuffer = stagingBuffer;
	copyBufferInfo.dstBuffer = vertexBuffer;

	Utils::CopyBuffer(copyBufferInfo);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	PROFILE_FUNCTION();

	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	Utils::CreateBuffer({ physicalDevice, device, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT , 
		&stagingBuffer, &stagingBufferMemory });

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices->data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	Utils::CreateBuffer({ physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT , &indexBuffer, &indexBufferMemory });

	Utils::CopyBuffer({device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize});

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

}
