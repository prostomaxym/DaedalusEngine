#pragma once

#include "Macros.h"
#include "DecodedImage.h"
#include "Model.h"
#include "Utils/VirtualFileSystem.h"
#include <string>
#include <vector>

namespace Daedalus
{
	enum class MaterialTextureSlot { Diffuse, Specular, Normal, Height };
	struct ImportedTextureBinding
	{
		MaterialTextureSlot slot;
		std::string virtual_path;
		std::shared_ptr<DecodedImage> image;
	};
	struct ImportedMaterial
	{
		std::string name;
		glm::vec3 ambient{ 1.f }, diffuse{ 1.f }, specular{ 1.f };
		float shininess = 16.f;
		std::vector<ImportedTextureBinding> textures;
	};
	struct ImportedMesh
	{
		std::string name;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t material_index = 0;
	};
	struct ImportedModel
	{
		std::vector<ImportedMesh> meshes;
		std::vector<ImportedMaterial> materials;
		std::vector<std::string> dependencies;
	};

	class DAEDALUS_API AssetImporter
	{
	public:
		[[nodiscard]] static std::shared_ptr<DecodedImage> ImportTexture(const VirtualFileSystem& files, const std::string& path, std::string& error, bool flip_vertically = true, int requested_channels = 0);
		[[nodiscard]] static bool ImportModel(const VirtualFileSystem& files, const std::string& path, ImportedModel& model, std::string& error,
			ModelParserFlags flags = ModelParserFlags::TRIANGULATE);
	};
}
