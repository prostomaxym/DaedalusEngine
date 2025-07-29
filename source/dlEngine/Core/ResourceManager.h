#pragma once

#include <memory>
#include <filesystem>

#include "Macros.h"

#include "Renderer/Resourses/ShaderLibrary.h"
#include "Renderer/Resourses/Model.h"

namespace Daedalus
{
	class DAEDALUS_API ResourceManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadShaders(const std::filesystem::path& path, bool recompile);
		static std::shared_ptr<Shader> GetShader(std::string_view name);

		static std::shared_ptr<Model> LoadModel(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE);

		static std::shared_ptr<Material> LoadMaterial(std::string_view material_name = "Default Material" , glm::vec3 kambient = { 1.f, 1.f, 0.1f }, glm::vec3 kdiffuse = { 1.f, 1.f, 1.f },
			glm::vec3 kspecular = { 1.f, 1.f, 1.f }, float shininess =  16.f );

		static std::shared_ptr<Mesh> LoadMesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index);

		static std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& path);
		static std::shared_ptr<Texture2D> LoadTexture2D(uint32_t width, uint32_t height, int channels = 4);	
		static std::shared_ptr<Texture2D> LoadTexture2D(unsigned char* data, int width, int height, int channels);
		static std::shared_ptr<Texture2D> LoadTexture2D(float* data, int width, int height, int channels);
		static std::shared_ptr<TextureCubemap> LoadCubemap(const std::vector<std::string>& faces);

	private:
		static std::unique_ptr<ShaderLibrary> s_shader_library;
		static std::unordered_map<std::string, std::shared_ptr<Model>> s_models;
		static std::unordered_map<std::string, std::shared_ptr<Material>> s_material;
		static std::unordered_map<std::string, std::shared_ptr<Texture2D>> s_named_textures;
		static std::set<std::shared_ptr<Texture2D>> s_unnamed_textures;
		static std::set<std::shared_ptr<TextureCubemap>> s_unnamed_cubemaps;
		static std::set<std::shared_ptr<Mesh>> s_meshes;

		static int s_vertex_count;
	};
}