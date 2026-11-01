#include "dlpch.h"

#include "Renderer/Resources/ResourceManager.h"
#include "Model.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "DecodedImage.h"
#include "Threads/JobSystem.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>

using namespace Daedalus;

Model::Model(const std::filesystem::path& path, ModelParserFlags parser_flags)
{
	if (!AssimpParser::LoadModel(path, m_meshes, m_material_data, parser_flags))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Model failed to load: {}", path.string());
		return;
	}

	ComputeBoundingSphere();
	ComputeBoundingAABB();
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Model loaded: {}", path.string());
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

void Model::ComputeBoundingAABB()
{
	if (m_meshes.empty())
		return;

	if (m_meshes.size() == 1)
	{
		m_bounding_aabb = m_meshes[0]->GetBoundingAABB();
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
			const auto& aabb = mesh->GetBoundingAABB();
			glm::vec3 aabbMin = aabb.GetMin();
			glm::vec3 aabbMax = aabb.GetMax();

			minX = std::min(minX, aabbMin.x);
			minY = std::min(minY, aabbMin.y);
			minZ = std::min(minZ, aabbMin.z);

			maxX = std::max(maxX, aabbMax.x);
			maxY = std::max(maxY, aabbMax.y);
			maxZ = std::max(maxZ, aabbMax.z);
		}

		glm::vec3 minPoint(minX, minY, minZ);
		glm::vec3 maxPoint(maxX, maxY, maxZ);
		m_bounding_aabb = AABB(minPoint, maxPoint);
	}
}

const std::vector<std::shared_ptr<Mesh>>& Model::GetMeshes() const
{
	return m_meshes;
}

const std::vector<std::shared_ptr<Material>>& Daedalus::Model::GetMaterials() const
{
	return m_material_data;
}

BoundingSphere Model::GetBoundingSphere() const
{
	return m_bounding_sphere;
}

AABB Model::GetBoundingAABB() const
{
	return m_bounding_aabb;
}

bool Model::IsVisible(const Frustum& frust, const glm::mat4& transform) const
{
	return frust.SphereInFrustum(m_bounding_sphere, transform);
}

bool AssimpParser::LoadModel(const std::filesystem::path& file_name, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<std::shared_ptr<Material>>& material_data, ModelParserFlags parser_flags)
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

void AssimpParser::ProcessMaterials(const aiScene* scene, std::vector<std::shared_ptr<Material>>& material_data, const std::filesystem::path& file_name)
{
	auto* jobs = ResourceManager::GetJobs();
	std::vector<JobHandle> uploads;

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

			material_data[i] = ResourceManager::LoadMaterial(name.C_Str()
				, glm::vec3(ambientColor.r, ambientColor.g, ambientColor.b)
				, glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b)
				, glm::vec3(specularColor.r, specularColor.g, specularColor.b)
				, shininess);

			std::array<aiTextureType, 3> texture_types{ aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_NORMALS };

			for (auto tex_type : texture_types)
			{
				for (uint32_t k = 0; k < material->GetTextureCount(tex_type); ++k)
				{
					aiString texturePath;
					if (material->GetTexture(tex_type, k, &texturePath) == AI_SUCCESS)
					{
						const auto source = file_name.parent_path() / texturePath.C_Str();
						auto image = std::make_shared<DecodedImage>();
						auto decode = [image, source]
						{
							if (!image->Load(source))
								Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Texture failed to decode: {}", source.string());
						};
						auto upload = [image, source, target = material_data[i], tex_type]
						{
							if (!image->GetPixels()) return;
							switch (tex_type)
							{
							case aiTextureType_DIFFUSE:
								target->SetDiffuseMap(source, image->GetPixels(), image->GetWidth(), image->GetHeight(), image->GetChannels());
								break;
							case aiTextureType_SPECULAR:
								target->SetSpecularMap(source, image->GetPixels(), image->GetWidth(), image->GetHeight(), image->GetChannels());
								break;
							case aiTextureType_NORMALS:
								target->SetNormalMap(source, image->GetPixels(), image->GetWidth(), image->GetHeight(), image->GetChannels());
								break;
							default:
								break;
							}
						};
						if (jobs)
						{
							// Preserve material texture ordering even when decoding finishes out of order.
							const std::array dependencies{ jobs->Submit(std::move(decode)), uploads.empty() ? JobHandle{} : uploads.back() };
							uploads.push_back(jobs->Submit(std::move(upload), dependencies, JobAffinity::MainThread));
						}
						else
						{
							decode();
							upload();
						}
					}
				}
			}
		}
		else
		{
			material_data[i] = ResourceManager::LoadMaterial();
		}
	}

	if (jobs && !jobs->Wait(jobs->WhenAll(uploads)))
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Model texture jobs failed: {}", file_name.string());
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
		meshes.push_back(ResourceManager::LoadMesh(mesh->mName.C_Str(), vertices, indices, mesh->mMaterialIndex));
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
	out_vertices.reserve(mesh->mNumVertices);
	out_indices.reserve(mesh->mNumVertices);

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
	m_bounding_sphere(other.m_bounding_sphere),
	m_bounding_aabb(other.m_bounding_aabb)
{
}

Model::Model(Model&& other) noexcept
	: m_meshes(std::move(other.m_meshes)),
	m_material_data(std::move(other.m_material_data)),
	m_bounding_sphere(std::move(other.m_bounding_sphere)),
	m_bounding_aabb(std::move(other.m_bounding_aabb))
{
}

Model& Model::operator=(const Model& other)
{
	if (this != &other)
	{
		m_meshes = other.m_meshes;
		m_material_data = other.m_material_data;
		m_bounding_sphere = other.m_bounding_sphere;
		m_bounding_aabb = other.m_bounding_aabb;
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
		m_bounding_aabb = std::move(other.m_bounding_aabb);
	}

	return *this;
}