#include "dlpch.h"
#include "ResourceState.h"
#include "CacheEviction.h"
#include "Utils/Utf8.h"

using namespace Daedalus;

namespace
{
	using Clock = std::chrono::steady_clock;
	double ElapsedMs(Clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}
}

ResourceManager* ResourceManager::s_instance = nullptr;

ResourceManager::ResourceManager() : m_state(std::make_unique<State>())
{
	DL_ASSERT(!s_instance, Log::Categories::Renderer, "Only one resource manager may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

ResourceManager::~ResourceManager()
{
	GetState();
	m_state.reset();
	s_instance = nullptr;
}

ResourceManager::State& ResourceManager::GetState()
{
	DL_ASSERT(s_instance, Log::Categories::Renderer, "ResourceManager used outside Engine lifetime");
	if (!s_instance) std::abort();
	auto& state = *s_instance->m_state;
	DL_ASSERT(state.m_owner == std::this_thread::get_id(), Log::Categories::Renderer, "GPU resources must be accessed on the graphics thread");
	if (state.m_owner != std::this_thread::get_id()) std::abort();
	return state;
}

void ResourceManager::LoadShaders(const std::filesystem::path& path, bool recompile)
{
	auto& state = GetState();
	state.m_shader_library = std::make_unique<ShaderLibrary>(path, recompile);
	++state.m_revision;
}

std::shared_ptr<Shader> ResourceManager::GetShader(std::string_view name)
{
	auto& state = GetState();
	return state.m_shader_library ? state.m_shader_library->Get(std::string(name)) : nullptr;
}

std::shared_ptr<Model> ResourceManager::LoadModel(const std::filesystem::path& path, ModelParserFlags parser_flags)
{
	auto& state = GetState();
	std::error_code ec;
	const auto normalized = std::filesystem::weakly_canonical(path, ec);
	if (ec || !std::filesystem::is_regular_file(normalized, ec) || ec) return nullptr;
	const auto key = Utf8ToString(normalized.generic_u8string()) + "|" + std::to_string(static_cast<uint32_t>(parser_flags));
	const auto found = state.m_models.find(key);
	if (found != state.m_models.end())
	{
		++state.m_model_cache.hits;
		return found->second;
	}
	const auto start = Clock::now();
	// Constructors are private; ResourceManager is their designated factory.
	auto model = std::shared_ptr<Model>(new Model(normalized, parser_flags));
	if (model->GetMeshes().empty()) return nullptr;
	state.m_models.emplace(key, model);
	state.m_load_times[model.get()] = ElapsedMs(start);
	++state.m_model_cache.misses;
	++state.m_revision;
	return model;
}

std::shared_ptr<Material> ResourceManager::LoadMaterial(std::string_view material_name, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess)
{
	auto& state = GetState();
	const std::string key(material_name);
	const auto found = state.m_material.find(key);
	if (found != state.m_material.end())
	{
		++state.m_material_cache.hits;
		return found->second;
	}
	auto material = std::shared_ptr<Material>(new Material(key, ambient, diffuse, specular, shininess));
	state.m_material.emplace(key, material);
	++state.m_material_cache.misses;
	++state.m_revision;
	return material;
}

std::shared_ptr<Mesh> ResourceManager::LoadMesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index)
{
	auto& state = GetState();
	auto mesh = std::shared_ptr<Mesh>(new Mesh(name, vertices, indices, material_index));
	state.m_meshes.insert(mesh);
	state.m_vertex_count += mesh->GetVertexCount();
	++state.m_revision;
	return mesh;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(const std::filesystem::path& path)
{
	auto& state = GetState();
	std::error_code ec;
	const auto normalized = std::filesystem::weakly_canonical(path, ec);
	if (ec || !std::filesystem::is_regular_file(normalized, ec) || ec) return nullptr;
	const auto key = Utf8ToString(normalized.generic_u8string());
	const auto found = state.m_named_textures.find(key);
	if (found != state.m_named_textures.end())
	{
		++state.m_texture_cache.hits;
		return found->second;
	}
	const auto start = Clock::now();
	auto texture = Texture2D::Create(normalized);
	if (!texture) return nullptr;
	state.m_named_textures.emplace(key, texture);
	state.m_load_times[texture.get()] = ElapsedMs(start);
	++state.m_texture_cache.misses;
	++state.m_revision;
	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(uint32_t width, uint32_t height, int channels)
{
	auto& state = GetState();
	const auto start = Clock::now();
	auto texture = Texture2D::Create(width, height, channels);
	if (!texture) return nullptr;
	state.m_unnamed_textures.insert(texture);
	state.m_load_times[texture.get()] = ElapsedMs(start);
	++state.m_revision;
	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(unsigned char* data, int width, int height, int channels)
{
	auto& state = GetState();
	const auto start = Clock::now();
	auto texture = Texture2D::Create(data, width, height, channels);
	if (!texture) return nullptr;
	state.m_unnamed_textures.insert(texture);
	state.m_load_times[texture.get()] = ElapsedMs(start);
	++state.m_revision;
	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(float* data, int width, int height, int channels)
{
	auto& state = GetState();
	const auto start = Clock::now();
	auto texture = Texture2D::Create(data, width, height, channels);
	if (!texture) return nullptr;
	state.m_unnamed_textures.insert(texture);
	state.m_load_times[texture.get()] = ElapsedMs(start);
	++state.m_revision;
	return texture;
}

std::shared_ptr<TextureCubemap> ResourceManager::LoadCubemap(const std::vector<std::string>& faces)
{
	auto& state = GetState();
	const auto start = Clock::now();
	auto cubemap = TextureCubemap::Create(faces);
	if (!cubemap || !cubemap->IsLoaded()) return nullptr;
	state.m_unnamed_cubemaps.insert(cubemap);
	state.m_load_times[cubemap.get()] = ElapsedMs(start);
	++state.m_revision;
	return cubemap;
}

size_t ResourceManager::PurgeUnused()
{
	auto& state = GetState();
	const auto mapped = [](const auto& entry) -> const auto& { return entry.second; };
	const auto identity = [](const auto& entry) -> const auto& { return entry; };
	const auto forget = [&state](const auto& resource) { state.m_load_times.erase(resource.get()); };
	// Parents first: releasing a model may make its meshes/materials/textures unused.
	size_t removed = EvictUnused(state.m_models, mapped, forget);
	removed += EvictUnused(state.m_meshes, identity, [&state, &forget](const auto& mesh)
	{
		state.m_vertex_count -= mesh->GetVertexCount();
		forget(mesh);
	});
	removed += EvictUnused(state.m_material, mapped, forget);
	removed += EvictUnused(state.m_named_textures, mapped, forget);
	removed += EvictUnused(state.m_unnamed_textures, identity, forget);
	removed += EvictUnused(state.m_unnamed_cubemaps, identity, forget);
	if (removed) ++state.m_revision;
	return removed;
}
