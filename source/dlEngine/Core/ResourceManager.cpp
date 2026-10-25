#include "dlpch.h"

#include "ResourceManager.h"

#include <algorithm>
#include <chrono>

using namespace Daedalus;

namespace
{
	using Clock = std::chrono::steady_clock;

	double ElapsedMs(Clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}

	template<typename T>
	long ExternalRefs(const std::shared_ptr<T>& ptr)
	{
		// The manager itself holds one reference
		return std::max(0L, static_cast<long>(ptr.use_count()) - 1L);
	}
}

std::unique_ptr<ShaderLibrary> ResourceManager::s_shader_library = nullptr;
std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::s_models = {};
std::unordered_map<std::string, std::shared_ptr<Material>> ResourceManager::s_material = {};
std::unordered_map<std::string, std::shared_ptr<Texture2D>> ResourceManager::s_named_textures = {};
std::set<std::shared_ptr<Texture2D>> ResourceManager::s_unnamed_textures = {};
std::set<std::shared_ptr<TextureCubemap>> ResourceManager::s_unnamed_cubemaps = {};
std::set<std::shared_ptr<Mesh>> ResourceManager::s_meshes = {};
int ResourceManager::s_vertex_count = 0;

std::recursive_mutex ResourceManager::s_mutex;
std::unordered_map<const void*, double> ResourceManager::s_load_times = {};
CacheCounters ResourceManager::s_model_cache = {};
CacheCounters ResourceManager::s_material_cache = {};
CacheCounters ResourceManager::s_texture_cache = {};
uint64_t ResourceManager::s_revision = 0;
uint64_t ResourceManager::s_cached_revision = 0;
bool ResourceManager::s_cached_stats_valid = false;
ResourceStats ResourceManager::s_cached_stats = {};

double ResourceManager::GetLoadTime(const void* resource)
{
	const auto it = s_load_times.find(resource);
	return it != s_load_times.end() ? it->second : 0.0;
}

ResourceStats ResourceManager::GetStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	// Totals only change when a resource is added or removed, so rebuild them lazily
	if (!s_cached_stats_valid || s_cached_revision != s_revision)
	{
		ResourceStats stats;
		stats.models = s_models.size();
		stats.materials = s_material.size();
		stats.meshes = s_meshes.size();
		stats.named_textures = s_named_textures.size();
		stats.unnamed_textures = s_unnamed_textures.size();
		stats.unnamed_cubemaps = s_unnamed_cubemaps.size();
		stats.vertices = s_vertex_count;
		stats.shaders = s_shader_library ? s_shader_library->Count() : 0;

		for (const auto& mesh : s_meshes)
		{
			stats.indices += mesh->GetIndexCount();
			stats.vertex_buffer_bytes += mesh->GetVertexBufferBytes();
			stats.index_buffer_bytes += mesh->GetIndexBufferBytes();
		}
		stats.triangles = stats.indices / 3;

		for (const auto& [name, texture] : s_named_textures)
		{
			if (texture)
				stats.texture_bytes += texture->GetMemorySizeBytes();
		}
		for (const auto& texture : s_unnamed_textures)
		{
			if (texture)
				stats.texture_bytes += texture->GetMemorySizeBytes();
		}
		for (const auto& cubemap : s_unnamed_cubemaps)
		{
			if (cubemap)
				stats.cubemap_bytes += cubemap->GetMemorySizeBytes();
		}

		s_cached_stats = stats;
		s_cached_revision = s_revision;
		s_cached_stats_valid = true;
	}

	auto stats = s_cached_stats;
	stats.model_cache = s_model_cache;
	stats.material_cache = s_material_cache;
	stats.texture_cache = s_texture_cache;
	return stats;
}

std::vector<ShaderStats> ResourceManager::GetShaderStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::vector<ShaderStats> result;
	if (!s_shader_library)
		return result;

	result.reserve(s_shader_library->Count());
	for (const auto& [name, shader] : s_shader_library->GetShaders())
	{
		ShaderStats entry;
		entry.name = name;
		entry.info = shader->GetInfo();
		if (const auto* load_info = s_shader_library->GetLoadInfo(name))
			entry.load = *load_info;

		result.push_back(std::move(entry));
	}

	std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
	return result;
}

std::vector<ModelStats> ResourceManager::GetModelStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::vector<ModelStats> result;
	result.reserve(s_models.size());

	for (const auto& [name, model] : s_models)
	{
		ModelStats entry;
		entry.name = name;
		entry.meshes = model->GetMeshes().size();
		entry.materials = model->GetMaterials().size();
		entry.ref_count = ExternalRefs(model);
		entry.load_time_ms = GetLoadTime(model.get());
		for (const auto& mesh : model->GetMeshes())
		{
			entry.vertices += mesh->GetVertexCount();
			entry.indices += mesh->GetIndexCount();
			entry.mesh_names.push_back(mesh->GetName());
		}
		result.push_back(std::move(entry));
	}

	std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
	return result;
}

std::vector<MeshStats> ResourceManager::GetMeshStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::vector<MeshStats> result;
	result.reserve(s_meshes.size());

	for (const auto& mesh : s_meshes)
	{
		MeshStats entry;
		entry.name = mesh->GetName();
		entry.vertices = mesh->GetVertexCount();
		entry.indices = mesh->GetIndexCount();
		entry.material_index = mesh->GetMaterialIndex();
		entry.vertex_buffer_bytes = mesh->GetVertexBufferBytes();
		entry.index_buffer_bytes = mesh->GetIndexBufferBytes();
		entry.bounding_radius = mesh->GetBoundingSphere().radius;

		const auto aabb = mesh->GetBoundingAABB();
		const auto min = aabb.GetMin();
		const auto max = aabb.GetMax();
		entry.aabb_min = { min.x, min.y, min.z };
		entry.aabb_max = { max.x, max.y, max.z };
		entry.ref_count = ExternalRefs(mesh);
		result.push_back(std::move(entry));
	}

	return result;
}

std::vector<MaterialStats> ResourceManager::GetMaterialStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::vector<MaterialStats> result;
	result.reserve(s_material.size());

	for (const auto& [name, material] : s_material)
	{
		MaterialStats entry;
		entry.name = name;

		const auto ambient = material->GetAmbientK();
		const auto diffuse = material->GetDiffuseK();
		const auto specular = material->GetSpecularK();
		entry.ambient = { ambient.x, ambient.y, ambient.z };
		entry.diffuse = { diffuse.x, diffuse.y, diffuse.z };
		entry.specular = { specular.x, specular.y, specular.z };
		entry.shininess = material->GetShininess();
		entry.ref_count = ExternalRefs(material);

		const auto add_map = [&entry](const char* slot, const std::shared_ptr<Texture2D>& texture)
			{
				if (!texture)
					return;

				entry.textures.push_back({ slot, texture->GetRendererID(), texture->GetWidth(), texture->GetHeight() });
			};

		add_map("Diffuse", material->GetDiffuseMap());
		add_map("Specular", material->GetSpecularMap());
		add_map("Normal", material->GetNormalMap());
		add_map("Height", material->GetHeightMap());

		result.push_back(std::move(entry));
	}

	std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
	return result;
}

std::vector<TextureStats> ResourceManager::GetTextureStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::unordered_map<const Texture2D*, std::vector<std::string>> used_by;
	for (const auto& [name, material] : s_material)
	{
		const std::shared_ptr<Texture2D> maps[] = { material->GetDiffuseMap(), material->GetSpecularMap(), material->GetNormalMap(), material->GetHeightMap() };
		for (const auto& map : maps)
		{
			if (map)
				used_by[map.get()].push_back(name);
		}
	}

	std::vector<TextureStats> result;
	result.reserve(s_named_textures.size() + s_unnamed_textures.size());

	const auto fill = [&used_by](const std::shared_ptr<Texture2D>& texture, std::string name, bool named)
		{
			TextureStats entry;
			entry.name = std::move(name);
			entry.path = texture->GetPath().string();
			entry.format = texture->GetFormatName();
			entry.width = texture->GetWidth();
			entry.height = texture->GetHeight();
			entry.mip_levels = texture->GetMipLevels();
			entry.renderer_id = texture->GetRendererID();
			entry.bytes = texture->GetMemorySizeBytes();
			entry.named = named;
			entry.ref_count = ExternalRefs(texture);
			entry.load_time_ms = GetLoadTime(texture.get());

			const auto it = used_by.find(texture.get());
			if (it != used_by.end())
				entry.used_by = it->second;

			return entry;
		};

	for (const auto& [name, texture] : s_named_textures)
	{
		if (texture)
			result.push_back(fill(texture, name, true));
	}
	for (const auto& texture : s_unnamed_textures)
	{
		if (texture)
			result.push_back(fill(texture, "<unnamed>", false));
	}

	return result;
}

std::vector<CubemapStats> ResourceManager::GetCubemapStats()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::vector<CubemapStats> result;
	result.reserve(s_unnamed_cubemaps.size());

	for (const auto& cubemap : s_unnamed_cubemaps)
	{
		if (!cubemap)
			continue;

		CubemapStats entry;
		entry.format = cubemap->GetFormatName();
		entry.face_width = cubemap->GetWidth();
		entry.face_height = cubemap->GetHeight();
		entry.renderer_id = cubemap->GetRendererID();
		entry.bytes = cubemap->GetMemorySizeBytes();
		entry.ref_count = ExternalRefs(cubemap);
		entry.load_time_ms = GetLoadTime(cubemap.get());
		result.push_back(std::move(entry));
	}

	return result;
}

void ResourceManager::Init()
{

}

void ResourceManager::Shutdown()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	s_shader_library.reset();
	s_models.clear();
	s_material.clear();
	s_named_textures.clear();
	s_unnamed_textures.clear();
	s_unnamed_cubemaps.clear();
	s_meshes.clear();

	s_vertex_count = 0;

	s_load_times.clear();
	s_model_cache = {};
	s_material_cache = {};
	s_texture_cache = {};
	s_revision++;
}

void ResourceManager::LoadShaders(const std::filesystem::path& path, bool recompile)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	s_shader_library = std::make_unique<ShaderLibrary>(path, recompile);
	s_revision++;
}

std::shared_ptr<Shader> ResourceManager::GetShader(std::string_view name)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	return s_shader_library->Get(name.data());
}


std::shared_ptr<Model> ResourceManager::LoadModel(const std::filesystem::path& path, ModelParserFlags parser_flags)
{
	// Recursive lock: model parsing calls back into LoadMesh/LoadMaterial/LoadTexture2D
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::error_code ec;
	const auto normalized_path = std::filesystem::weakly_canonical(path, ec);
	if (ec || !std::filesystem::is_regular_file(normalized_path, ec) || ec) return nullptr;
	const auto name = normalized_path.generic_u8string() + "|" + std::to_string(static_cast<uint32_t>(parser_flags));
	auto it = s_models.find(name);

	if (it == s_models.end())
	{
		const auto start = Clock::now();

		auto model = std::shared_ptr<Model>(new Model(normalized_path, parser_flags));
		if (model->GetMeshes().empty()) return nullptr;
		s_models[name] = model;
		s_load_times[model.get()] = ElapsedMs(start);
		s_model_cache.misses++;
		s_revision++;

		return model;
	}
	else
	{
		s_model_cache.hits++;
		return it->second;
	}
}

std::shared_ptr<Material> ResourceManager::LoadMaterial(std::string_view material_name, glm::vec3 kambient, glm::vec3 kdiffuse,
	glm::vec3 kspecular, float shininess)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	std::string key(material_name);
	auto it = s_material.find(key);

	if (it == s_material.end())
	{
		auto material = std::shared_ptr<Material>(new Material(key, kambient, kdiffuse, kspecular, shininess));
		s_material[key] = material;
		s_material_cache.misses++;
		s_revision++;
		return material;
	}
	else
	{
		s_material_cache.hits++;
		return it->second;
	}
}

std::shared_ptr<Mesh> ResourceManager::LoadMesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	auto mesh = std::shared_ptr<Mesh>(new Mesh(name, vertices, indices, material_index));
	s_meshes.insert(mesh);

	s_vertex_count += mesh->GetVertexCount();
	s_revision++;

	return mesh;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(const std::filesystem::path& path)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	const auto name = path.string();
	auto it = s_named_textures.find(name);

	if (it == s_named_textures.end())
	{
		const auto start = Clock::now();

		auto texture = Texture2D::Create(path);
		s_named_textures[name] = texture;
		s_load_times[texture.get()] = ElapsedMs(start);
		s_texture_cache.misses++;
		s_revision++;

		return texture;
	}
	else
	{
		s_texture_cache.hits++;
		return it->second;
	}
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(uint32_t width, uint32_t height, int channels)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	const auto start = Clock::now();

	auto texture = Texture2D::Create(width, height, channels);
	s_unnamed_textures.insert(texture);
	s_load_times[texture.get()] = ElapsedMs(start);
	s_revision++;

	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(unsigned char* data, int width, int height, int channels)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	const auto start = Clock::now();

	auto texture = Texture2D::Create(data, width, height, channels);
	s_unnamed_textures.insert(texture);
	s_load_times[texture.get()] = ElapsedMs(start);
	s_revision++;

	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(float* data, int width, int height, int channels)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	const auto start = Clock::now();

	auto texture = Texture2D::Create(data, width, height, channels);
	s_unnamed_textures.insert(texture);
	s_load_times[texture.get()] = ElapsedMs(start);
	s_revision++;

	return texture;
}

std::shared_ptr<TextureCubemap> ResourceManager::LoadCubemap(const std::vector<std::string>& faces)
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	const auto start = Clock::now();

	auto cubemap = TextureCubemap::Create(faces);
	if (!cubemap || !cubemap->IsLoaded()) return nullptr;
	s_unnamed_cubemaps.insert(cubemap);
	s_load_times[cubemap.get()] = ElapsedMs(start);
	s_revision++;

	return cubemap;
}
