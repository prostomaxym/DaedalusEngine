#pragma once

#include <memory>
#include <filesystem>

#include "Macros.h"

#include "Renderer/Resourses/ShaderLibrary.h"
#include "Renderer/Resourses/Model.h"
#include "ResourceStats.h"


namespace Daedalus
{
	class DAEDALUS_API ResourceManager
	{
	public:
		static std::vector<ShaderStats> GetShaderStats();
		static ResourceStats GetStats();
		static std::vector<ModelStats> GetModelStats();
		static std::vector<MeshStats> GetMeshStats();
		static std::vector<MaterialStats> GetMaterialStats();
		static std::vector<TextureStats> GetTextureStats();
		static std::vector<CubemapStats> GetCubemapStats();

		ResourceManager();
		~ResourceManager();
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

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
		// Call at scene transitions on the graphics thread, not on every frame.
		static size_t PurgeUnused();

	private:
		static double GetLoadTime(const void* resource);
		struct State;
		static State& GetState();
		std::unique_ptr<State> m_state;
		static ResourceManager* s_instance; // Non-owning; Engine owns the manager.
	};
}
