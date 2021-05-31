#include "RenderPipelineBase.h"

VkPipeline Renderer::RenderPipelineBase::GetPipeline() const
{
	return gfxPipeline;
}

VkPipelineLayout Renderer::RenderPipelineBase::GetPipelineLayout() const
{
	return pipelineLayout;
}

VkDescriptorSet& Renderer::RenderPipelineBase::GetDescriptorSet(uint32_t index)
{
	return (descriptorSets[index]);
}

VkDescriptorSet& Renderer::RenderPipelineBase::GetSamplerDescriptorSet(uint32_t index)
{
	return samplerDescriptorSets[index];
}

VkDescriptorSet& Renderer::RenderPipelineBase::GetInputDescriptorSet(uint32_t index)
{
	return inputDescriptorSets[index];
}

void Renderer::RenderPipelineBase::SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	PROFILE_FUNCTION();

	uboViewProjection.projection = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	uboViewProjection.projection[1][1] *= -1.0f;
}

void Renderer::RenderPipelineBase::SetViewMatrixFromLookAt(const glm::vec3& location, const glm::vec3& lookAt, const glm::vec3& upVec)
{
	uboViewProjection.view = glm::lookAt(location, lookAt, upVec);
}
