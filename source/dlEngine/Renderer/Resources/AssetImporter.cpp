#include "dlpch.h"
#include "AssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <cstring>
#include <set>
#include <cmath>

namespace Daedalus
{
	namespace
	{
		class MemoryStream final : public Assimp::IOStream
		{
		public:
			explicit MemoryStream(std::string bytes) : m_bytes(std::move(bytes)) {}
			size_t Read(void* buffer, size_t size, size_t count) override
			{
				if (!size) return 0;
				const auto elements = (std::min)(count, (m_bytes.size() - m_position) / size);
				std::memcpy(buffer, m_bytes.data() + m_position, elements * size);
				m_position += elements * size;
				return elements;
			}
			size_t Write(const void*, size_t, size_t) override { return 0; }
			aiReturn Seek(size_t offset, aiOrigin origin) override
			{
				size_t position = 0;
				if (origin == aiOrigin_SET) position = offset;
				else if (origin == aiOrigin_CUR)
				{
					if (offset > m_bytes.size() - m_position) return aiReturn_FAILURE;
					position = m_position + offset;
				}
				else if (origin == aiOrigin_END)
				{
					if (offset > m_bytes.size()) return aiReturn_FAILURE;
					position = m_bytes.size() - offset;
				}
				else return aiReturn_FAILURE;
				if (position > m_bytes.size()) return aiReturn_FAILURE;
				m_position = position;
				return aiReturn_SUCCESS;
			}
			size_t Tell() const override { return m_position; }
			size_t FileSize() const override { return m_bytes.size(); }
			void Flush() override {}
		private:
			std::string m_bytes;
			size_t m_position = 0;
		};

		class AssetIO final : public Assimp::IOSystem
		{
		public:
			AssetIO(const VirtualFileSystem& files, std::string source, std::set<std::string>& dependencies)
				: m_files(files), m_source(std::move(source)), m_dependencies(dependencies) {}
			bool Exists(const char* path) const override
			{
				const auto name = VirtualFileSystem::Join(m_source, path);
				std::string data, error;
				return name && m_files.Read(*name, data, error);
			}
			char getOsSeparator() const override { return '/'; }
			Assimp::IOStream* Open(const char* path, const char* mode = "rb") override
			{
				if (!path || !mode || std::strpbrk(mode, "w+a")) return nullptr;
				const auto name = VirtualFileSystem::Join(m_source, path);
				std::string data, error;
				if (!name || !m_files.Read(*name, data, error)) return nullptr;
				m_dependencies.insert(*name);
				// Assimp owns streams and releases them through Close.
				return std::make_unique<MemoryStream>(std::move(data)).release();
			}
			void Close(Assimp::IOStream* stream) override { std::unique_ptr<Assimp::IOStream> owned(stream); }
		private:
			const VirtualFileSystem& m_files;
			std::string m_source;
			std::set<std::string>& m_dependencies;
		};

		bool ReadMeshes(const aiScene& scene, const aiNode& node, const aiMatrix4x4& parent, ImportedModel& result)
		{
			const auto transform = parent * node.mTransformation;
			const aiMatrix3x3 directions(transform);
			aiMatrix3x3 normals(directions);
			normals.Inverse().Transpose();
			for (unsigned int index = 0; index < node.mNumMeshes; ++index)
			{
				const auto& source = *scene.mMeshes[node.mMeshes[index]];
				if (source.mNumVertices > 10000000 || source.mNumFaces > 10000000) return false;
				ImportedMesh mesh;
				mesh.name = source.mName.C_Str(); mesh.material_index = source.mMaterialIndex;
				mesh.vertices.reserve(source.mNumVertices);
				for (unsigned int vertex = 0; vertex < source.mNumVertices; ++vertex)
				{
					const auto position = transform * source.mVertices[vertex];
					const auto normal = source.mNormals ? normals * source.mNormals[vertex] : aiVector3D();
					const auto uv = source.mTextureCoords[0] ? source.mTextureCoords[0][vertex] : aiVector3D();
					const auto tangent = source.mTangents ? directions * source.mTangents[vertex] : aiVector3D();
					const auto bitangent = source.mBitangents ? directions * source.mBitangents[vertex] : aiVector3D();
					if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
						!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) return false;
					mesh.vertices.push_back(Vertex{ { position.x, position.y, position.z }, { uv.x, uv.y }, { normal.x, normal.y, normal.z },
						{ tangent.x, tangent.y, tangent.z }, { bitangent.x, bitangent.y, bitangent.z } });
				}
				for (unsigned int face = 0; face < source.mNumFaces; ++face)
				{
					const auto& triangle = source.mFaces[face];
					if (triangle.mNumIndices != 3) return false;
					for (unsigned int element = 0; element < 3; ++element)
					{
						if (triangle.mIndices[element] >= source.mNumVertices) return false;
						mesh.indices.push_back(triangle.mIndices[element]);
					}
				}
				if (!mesh.vertices.empty() && !mesh.indices.empty()) result.meshes.push_back(std::move(mesh));
			}
			for (unsigned int child = 0; child < node.mNumChildren; ++child)
				if (!ReadMeshes(scene, *node.mChildren[child], transform, result)) return false;
			return true;
		}
	}

	std::shared_ptr<DecodedImage> AssetImporter::ImportTexture(const VirtualFileSystem& files, const std::string& path, std::string& error, bool flip_vertically, int requested_channels)
	{
		std::string bytes;
		if (!files.Read(path, bytes, error)) return nullptr;
		auto image = std::make_shared<DecodedImage>();
		if (!image->LoadMemory({ reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size() }, flip_vertically, requested_channels))
		{ error = "Cannot decode texture: " + path; return nullptr; }
		return image;
	}

	bool AssetImporter::ImportModel(const VirtualFileSystem& files, const std::string& path, ImportedModel& model, std::string& error, ModelParserFlags flags)
	{
		error.clear();
		const auto normalized = VirtualFileSystem::Normalize(path);
		if (!normalized) { error = "Invalid model virtual path"; return false; }
		std::set<std::string> dependencies;
		Assimp::Importer importer;
		// Assimp owns its I/O adapter after SetIOHandler.
		importer.SetIOHandler(std::make_unique<AssetIO>(files, *normalized, dependencies).release());
		const auto* scene = importer.ReadFile(*normalized, static_cast<unsigned int>(flags) | aiProcess_Triangulate);
		if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
		{ error = importer.GetErrorString(); return false; }
		ImportedModel result;
		for (unsigned int index = 0; index < scene->mNumMaterials; ++index)
		{
			const auto& source = *scene->mMaterials[index];
			ImportedMaterial material;
			aiString name;
			source.Get(AI_MATKEY_NAME, name); material.name = name.C_Str();
			aiColor3D color(1.f);
			if (source.Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) material.ambient = { color.r, color.g, color.b };
			if (source.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) material.diffuse = { color.r, color.g, color.b };
			if (source.Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) material.specular = { color.r, color.g, color.b };
			source.Get(AI_MATKEY_SHININESS, material.shininess);
			for (const auto& [type, slot] : { std::pair{ aiTextureType_DIFFUSE, MaterialTextureSlot::Diffuse },
				std::pair{ aiTextureType_SPECULAR, MaterialTextureSlot::Specular }, std::pair{ aiTextureType_NORMALS, MaterialTextureSlot::Normal },
				std::pair{ aiTextureType_HEIGHT, MaterialTextureSlot::Height } })
			{
				for (unsigned int texture = 0; texture < source.GetTextureCount(type); ++texture)
				{
					aiString reference;
					if (source.GetTexture(type, texture, &reference) != AI_SUCCESS) continue;
					std::shared_ptr<DecodedImage> image;
					std::string texture_path;
					if (const auto* embedded = scene->GetEmbeddedTexture(reference.C_Str()))
					{
						texture_path = *normalized + "#embedded-" + reference.C_Str();
						image = std::make_shared<DecodedImage>();
						bool loaded = false;
						if (embedded->mHeight == 0)
							loaded = image->LoadMemory({ reinterpret_cast<const unsigned char*>(embedded->pcData), embedded->mWidth });
						else if (static_cast<uint64_t>(embedded->mWidth) * embedded->mHeight <= 64ull * 1024 * 1024)
						{
							std::vector<unsigned char> pixels(static_cast<size_t>(embedded->mWidth) * embedded->mHeight * 4);
							for (size_t pixel = 0; pixel < pixels.size() / 4; ++pixel)
							{
								const auto& value = embedded->pcData[pixel];
								pixels[pixel * 4] = value.r; pixels[pixel * 4 + 1] = value.g; pixels[pixel * 4 + 2] = value.b; pixels[pixel * 4 + 3] = value.a;
							}
							loaded = image->LoadRGBA(pixels, embedded->mWidth, embedded->mHeight);
						}
						if (!loaded) { error = "Cannot decode embedded model texture"; return false; }
					}
					else
					{
						const auto joined = VirtualFileSystem::Join(*normalized, reference.C_Str());
						if (!joined) { error = "Invalid model texture reference"; return false; }
						texture_path = *joined;
						dependencies.insert(texture_path);
						image = ImportTexture(files, texture_path, error);
						if (!image) return false;
					}
					material.textures.push_back({ slot, texture_path, std::move(image) });
				}
			}
			result.materials.push_back(std::move(material));
		}
		if (!ReadMeshes(*scene, *scene->mRootNode, aiMatrix4x4(), result) || result.meshes.empty())
		{ error = "Model contains no valid triangle geometry"; return false; }
		result.dependencies.assign(dependencies.begin(), dependencies.end());
		model = std::move(result);
		return true;
	}
}
