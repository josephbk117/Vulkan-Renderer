#pragma once
#include <vector>
#include "Mesh.h"
#include "assimp/scene.h"

class Model
{
public:
	Model();
	Model(const std::vector<Mesh>& meshList);
	size_t GetMeshCount() const;
	const Mesh* GetMesh(size_t index)const;
	const glm::mat4& GetModelMatrix() const;
	void SetModelMatrix(const glm::mat4& modelMatrix);
	void DestroyModel();
	~Model();

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, 
		VkQueue transferQueue, VkCommandPool transferCmdPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex, float scaleFactor);
	static Mesh LoadMesh(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, 
		VkQueue transferQueue, VkCommandPool transferCmdPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex, float scaleFactor);

private:
	std::vector<Mesh> meshList;
	glm::mat4 model;
};

