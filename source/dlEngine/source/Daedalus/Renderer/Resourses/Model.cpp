#include "dlpch.h"
#include "Model.h"
#include "Texture.h"
#include "Daedalus/Threads/DaedalusThreads.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <stb_image.h>

using namespace Daedalus;

Model::Model(const std::filesystem::path& path, ModelParserFlags parser_flags)
{
	if (!AssimpParser::LoadModel(path, m_meshes, m_material_data, parser_flags))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Model failed to load: " + path.string());
		throw std::runtime_error("Model failed to load: " + path.string());
	}

	ComputeBoundingSphere();
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Model loaded: " + path.string());
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

const std::vector<Material>& Daedalus::Model::GetMaterials() const
{
	return m_material_data;
}

BoundingSphere Model::GetBoundingSphere() const
{
	return m_bounding_sphere;
}

bool AssimpParser::LoadModel(const std::filesystem::path& file_name, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<Material>& material_data, ModelParserFlags parser_flags)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(file_name.string(), static_cast<unsigned int>(parser_flags));

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return false;

	ProcessMaterials(scene, material_data, file_name);

	aiMatrix4x4 identity;

	ProcessNode(&identity, scene->mRootNode, scene, meshes);

	return true;
}

void AssimpParser::ProcessMaterials(const aiScene* scene, std::vector<Material>& material_data, const std::filesystem::path& file_name)
{
	std::vector<std::future<std::tuple<unsigned char*, int, int, int, uint32_t, uint32_t>>> futures;

	material_data.resize(scene->mNumMaterials);

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* material = scene->mMaterials[i];
		if (material)
		{
			aiString name;
			aiColor3D ambientColor, diffuseColor, specularColor;
			float shininess;

			material->Get(AI_MATKEY_NAME, name);
			material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
			material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
			material->Get(AI_MATKEY_SHININESS, shininess);

			material_data[i] = Material(name.C_Str()
				, glm::vec3(ambientColor.r, ambientColor.g, ambientColor.b)
				, glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b)
				, glm::vec3(specularColor.r, specularColor.g, specularColor.b)
				, shininess);

			std::array<aiTextureType, 3> texture_types{ aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_NORMALS };

			for(auto tex_type : texture_types)
			{
				for (uint32_t k = 0; k < material->GetTextureCount(tex_type); ++k)
				{
					aiString texturePath;
					if (material->GetTexture(tex_type, k, &texturePath) == AI_SUCCESS)
					{
						std::string fullPath = file_name.parent_path().string() + "/" + texturePath.C_Str();

						futures.push_back(DaedalusThreads::Inst().SubmitAndReturnFuture([fullPath, mat_num = i, tex_type]()->std::tuple<unsigned char*, int, int, int, uint32_t, uint32_t>
							{
								int width, height, channels;
								stbi_set_flip_vertically_on_load(1);
								stbi_uc* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

								return { data, width, height, channels, mat_num, tex_type };
							}));
					}
				}
			}
		}
		else
		{
			material_data[i] = Material();
		}
	}

	for (auto& fut : futures)
	{
		fut.wait();
		const auto& future = fut.get();

		const aiTextureType tex_type = static_cast<aiTextureType>(std::get<5>(future));
		const auto material_index = std::get<4>(future);

		switch (tex_type)
		{
		case aiTextureType_DIFFUSE:
			material_data[material_index].SetDiffuseMap(std::get<0>(future), std::get<1>(future), std::get<2>(future), std::get<3>(future));
			break;

		case aiTextureType_SPECULAR:
			material_data[material_index].SetSpecularMap(std::get<0>(future), std::get<1>(future), std::get<2>(future), std::get<3>(future));
			break;

		case aiTextureType_NORMALS:
			material_data[material_index].SetNormalMap(std::get<0>(future), std::get<1>(future), std::get<2>(future), std::get<3>(future));
			break;

		default:
			break;
		}

		stbi_image_free(std::get<0>(future));
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

Model::Model(const Model& other)
	: m_meshes(other.m_meshes),
	m_material_data(other.m_material_data),
	m_bounding_sphere(other.m_bounding_sphere)
{
}

Model::Model(Model&& other) noexcept
	: m_meshes(std::move(other.m_meshes)),
	m_material_data(std::move(other.m_material_data)),
	m_bounding_sphere(std::move(other.m_bounding_sphere))
{
}

Model& Model::operator=(const Model& other)
{
	if (this != &other)
	{
		m_meshes = other.m_meshes;
		m_material_data = other.m_material_data;
		m_bounding_sphere = other.m_bounding_sphere;
	}

	return *this;
}

Model& Model::operator=(Model&& other) noexcept
{
	if (this != &other)
	{
		m_meshes = std::move(other.m_meshes);
		m_material_data = std::move(other.m_material_data);
		m_bounding_sphere = std::move(other.m_bounding_sphere);
	}

	return *this;
}