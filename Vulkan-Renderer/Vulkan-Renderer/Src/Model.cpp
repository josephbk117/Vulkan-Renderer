#include "Model.h"

Model::Model()
{
	model = glm::mat4(1.0f);
}

Model::Model(const std::vector<Mesh>& meshList)
{
	this->meshList = meshList;
	model = glm::mat4(1.0f);
}

size_t Model::GetMeshCount() const
{
	return meshList.size();
}

const Mesh* Model::GetMesh(size_t index) const
{
	if (index >= meshList.size())
	{
		throw std::runtime_error("Tried to access invalid index of mesh in model");
	}
	return &meshList[index];
}

glm::mat4& Model::GetModelMatrix()
{
	return model;
}

void Model::SetModelMatrix(const glm::mat4& modelMatrix)
{
	model = modelMatrix;
}

void Model::DestroyModel()
{
	PROFILE_FUNCTION();

	for (auto& mesh : meshList)
	{
		mesh.DestroyBuffers();
	}
}

Model::~Model()
{

}

std::vector<std::string> Model::LoadMaterials(const aiScene* scene)
{
	PROFILE_FUNCTION();

	std::vector<std::string> textureList(scene->mNumMaterials);

	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		textureList[i] = "";

		if (material != nullptr && material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS)
			{
				const std::string pathData = std::string(path.data);
				size_t idx = pathData.rfind("\\");
				std::string fileName = std::string(pathData).substr(idx + 1);

				textureList[i] = fileName;
			}
		}
	}

	return textureList;
}

std::vector<Mesh> Model::LoadNode(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCmdPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex, float scaleFactor)
{
	PROFILE_FUNCTION();

	std::vector<Mesh> meshList;

	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		meshList.push_back(LoadMesh(physicalDevice, logicalDevice, transferQueue, transferCmdPool, scene->mMeshes[node->mMeshes[i]], scene, matToTex, scaleFactor));
	}

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(physicalDevice, logicalDevice, transferQueue, transferCmdPool, node->mChildren[i], scene, matToTex, scaleFactor);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

Mesh Model::LoadMesh(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCmdPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex, float scaleFactor)
{
	PROFILE_FUNCTION();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.resize(mesh->mNumVertices);

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vertices[i].pos *= scaleFactor;

		if (mesh->mColors[0] != nullptr)
		{
			vertices[i].col = { mesh->mColors[0][i].r,  mesh->mColors[0][i].g , mesh->mColors[0][i].b };
		}
		else
		{
			vertices[i].col = { 1.0f, 1.0f, 1.0f };
		}

		if (mesh->mTextureCoords[0] != nullptr)
		{
			vertices[i].uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].uv = { 0, 0 };
		}
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	Mesh newMesh = Mesh(physicalDevice, logicalDevice, transferQueue, transferCmdPool, &vertices, &indices, matToTex[mesh->mMaterialIndex]);

	return newMesh;

}
