#pragma once

#include <memory>
#include <filesystem>

#include "Macros.h"

#include "Renderer/Resources/Model.h"
#include "ResourceStats.h"
#include <string_view>
#include "Core/AssetRegistry.h"
#include "Threads/JobSystem.h"
#include <functional>


namespace Daedalus
{
	class Texture2D;
	class TextureCubemap;
	class JobSystem;
	class AssetPipeline;
	struct ImportedModel;
	class DecodedImage;
	template<typename T> struct ResourceLoadResult
	{
		std::shared_ptr<T> resource;
		std::string error;
		[[nodiscard]] bool Succeeded() const { return resource != nullptr && error.empty(); }
	};
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

		explicit ResourceManager(JobSystem* jobs = nullptr, AssetPipeline* assets = nullptr);
		static JobSystem* GetJobs();
		~ResourceManager();
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		static void LoadShaders(const std::filesystem::path& path, bool recompile);
		static void ReloadShaders(AssetPipeline& assets);
		static std::shared_ptr<Shader> GetShader(std::string_view name);
		static std::shared_ptr<Shader> TryGetShader(std::string_view name);

		static std::shared_ptr<Model> LoadModel(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE);
		static std::shared_ptr<Model> LoadModelAsset(std::string_view path, ModelParserFlags flags = ModelParserFlags::TRIANGULATE);
		static std::shared_ptr<Model> LoadModelAsset(AssetId id, ModelParserFlags flags = ModelParserFlags::TRIANGULATE);
		static std::shared_ptr<Texture2D> LoadTextureAsset(std::string_view path);
		static std::shared_ptr<Texture2D> LoadTextureAsset(AssetId id);
		[[nodiscard]] static JobHandle LoadModelAsync(std::string_view path, std::function<void(ResourceLoadResult<Model>)> callback, ModelParserFlags flags = ModelParserFlags::TRIANGULATE);
		[[nodiscard]] static JobHandle LoadModelAsync(AssetId id, std::function<void(ResourceLoadResult<Model>)> callback, ModelParserFlags flags = ModelParserFlags::TRIANGULATE);
		[[nodiscard]] static JobHandle LoadTextureAsync(std::string_view path, std::function<void(ResourceLoadResult<Texture2D>)> callback);
		[[nodiscard]] static JobHandle LoadTextureAsync(AssetId id, std::function<void(ResourceLoadResult<Texture2D>)> callback);
		static void ReloadAssets(const std::vector<std::string>& changed);
		static std::shared_ptr<TextureCubemap> LoadCubemapAsset(const std::vector<std::string>& faces);
		[[nodiscard]] static JobHandle LoadCubemapAsync(const std::vector<std::string>& faces, std::function<void(ResourceLoadResult<TextureCubemap>)> callback);

		static std::shared_ptr<Material> LoadMaterial(std::string_view material_name = "Default Material" , glm::vec3 kambient = { 1.f, 1.f, 0.1f }, glm::vec3 kdiffuse = { 1.f, 1.f, 1.f },
			glm::vec3 kspecular = { 1.f, 1.f, 1.f }, float shininess =  16.f );

		static std::shared_ptr<Mesh> LoadMesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index);

		static std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& path);
		static std::shared_ptr<Texture2D> LoadTexture2D(uint32_t width, uint32_t height, int channels = 4);	
		static std::shared_ptr<Texture2D> LoadTexture2D(unsigned char* data, int width, int height, int channels);
		// Registers pixels already decoded from source under the source file name; returns the cached texture if present
		static std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& source, unsigned char* data, int width, int height, int channels);
		static std::shared_ptr<Texture2D> LoadTexture2D(float* data, int width, int height, int channels);
		static std::shared_ptr<TextureCubemap> LoadCubemap(const std::vector<std::string>& faces);
		// Call at scene transitions on the graphics thread, not on every frame.
		static size_t PurgeUnused();

	private:
		static std::shared_ptr<Texture2D> UploadTexture(const std::string& path, DecodedImage& image, std::string& error);
		static std::shared_ptr<Model> UploadModel(const std::string& key, ImportedModel& model, std::string& error);
		static double GetLoadTime(const void* resource);
		struct State;
		static State& GetState();
		std::unique_ptr<State> m_state;
		JobSystem* m_jobs = nullptr; // Non-owning; Engine owns the scheduler and drains it before this manager.
		AssetPipeline* m_assets = nullptr; // Non-owning; Engine owns the pipeline and outlives this manager.
		static ResourceManager* s_instance; // Non-owning; Engine owns the manager.
	};
}
