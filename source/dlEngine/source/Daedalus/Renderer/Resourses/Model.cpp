#include "dlpch.h"
#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>

using namespace Daedalus;

Model::Model(const std::string& path, ModelParserFlags parser_flags)
{
	if (!AssimpParser::LoadModel(path, m_meshes, m_material_names, parser_flags))
	{
		DL_ERROR("Model failed to load: " + path);
		throw std::runtime_error("Model failed to load: " + path);
	}

	ComputeBoundingSphere();
	DL_INFO("Model loaded: " + path);
}

void Model::ComputeBoundingSphere()
{
	if (m_meshes.empty())
		return;

	if (m_meshes.size() == 1)
	{
		m_bounding_sphere = m_meshes[0]->GetBoundingSphere();
	}
	else
	{
		float minX = std::numeric_limits<float>::max();
		float minY = std::numeric_limits<float>::max();
		float minZ = std::numeric_limits<float>::max();

		float maxX = std::numeric_limits<float>::min();
		float maxY = std::numeric_limits<float>::min();
		float maxZ = std::numeric_limits<float>::min();

		for (const auto& mesh : m_meshes)
		{
			const auto& boundingSphere = mesh->GetBoundingSphere();
			minX = std::min(minX, boundingSphere.position.x - boundingSphere.radius);
			minY = std::min(minY, boundingSphere.position.y - boundingSphere.radius);
			minZ = std::min(minZ, boundingSphere.position.z - boundingSphere.radius);

			maxX = std::max(maxX, boundingSphere.position.x + boundingSphere.radius);
			maxY = std::max(maxY, boundingSphere.position.y + boundingSphere.radius);
			maxZ = std::max(maxZ, boundingSphere.position.z + boundingSphere.radius);
		}

		m_bounding_sphere.position = glm::vec3{ minX + maxX, minY + maxY, minZ + maxZ } / 2.0f;
		m_bounding_sphere.radius = glm::distance(m_bounding_sphere.position, { minX, minY, minZ });
	}
}

const std::vector<std::shared_ptr<Mesh>>& Model::GetMeshes() const
{
	return m_meshes;
}

const std::vector<std::string>& Model::GetMaterialNames() const
{
	return m_material_names;
}

const BoundingSphere Model::GetBoundingSphere() const
{
	return m_bounding_sphere;
}

bool AssimpParser::LoadModel(const std::string& file_name, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<std::string>& materials, ModelParserFlags parser_flags)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(file_name, static_cast<unsigned int>(parser_flags));

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return false;

	ProcessMaterials(scene, materials);

	aiMatrix4x4 identity;

	ProcessNode(&identity, scene->mRootNode, scene, meshes);

	return true;
}

void AssimpParser::ProcessMaterials(const aiScene * scene, std::vector<std::string>&materials)
{
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* material = scene->mMaterials[i];
		if (material)
		{
			aiString name;
			aiGetMaterialString(material, AI_MATKEY_NAME, &name);
			materials.push_back(name.C_Str());
		}
	}
}

void AssimpParser::ProcessNode(void* transform, aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes)
{
	aiMatrix4x4 nodeTransformation = *reinterpret_cast<aiMatrix4x4*>(transform) * node->mTransformation;

	// Process all the node's meshes (if any)
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(&nodeTransformation, mesh, scene, vertices, indices);
		meshes.push_back(std::make_unique<Mesh>(vertices, indices, mesh->mMaterialIndex));
	}

	// Then do the same for each of its children
	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(&nodeTransformation, node->mChildren[i], scene, meshes);
	}
}

void AssimpParser::ProcessMesh(void* transform, aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices)
{
	aiMatrix4x4 mesh_transformation = *reinterpret_cast<aiMatrix4x4*>(transform);

	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		aiVector3D position = mesh_transformation * mesh->mVertices[i];
		aiVector3D normal = mesh_transformation * (mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f));
		aiVector3D tex_coords = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);
		aiVector3D tangent = mesh->mTangents ? mesh_transformation * mesh->mTangents[i] : aiVector3D(0.0f, 0.0f, 0.0f);
		aiVector3D bitangent = mesh->mBitangents ? mesh_transformation * mesh->mBitangents[i] : aiVector3D(0.0f, 0.0f, 0.0f);

		out_vertices.push_back(
			Vertex
			{
				{ position.x, position.y, position.z },
				{ tex_coords.x, tex_coords.y },
				{ normal.x, normal.y, normal.z },
				{ tangent.x, tangent.y, tangent.z },
				{ bitangent.x, bitangent.y, bitangent.z }
			}
		);
	}

	for (uint32_t faceID = 0; faceID < mesh->mNumFaces; ++faceID)
	{
		auto& face = mesh->mFaces[faceID];

		for (size_t indexID = 0; indexID < 3; ++indexID)
			out_indices.push_back(face.mIndices[indexID]);
	}
}