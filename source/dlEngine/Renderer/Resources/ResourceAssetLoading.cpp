#include "dlpch.h"
#include "ResourceState.h"
#include "AssetImporter.h"
#include "Core/AssetPipeline.h"
#include "Utils/Utf8.h"
#include <array>
#include <algorithm>

using namespace Daedalus;

namespace
{
	bool Unchanged(const AssetRegistry& before, const AssetRegistry& now, const std::vector<std::string>& paths)
	{
		for (const auto& path : paths)
		{
			const auto* old = before.Find(path);
			const auto* current = now.Find(path);
			if ((old == nullptr) != (current == nullptr) || (old && (old->id != current->id || old->content_hash != current->content_hash))) return false;
		}
		return true;
	}
}

std::shared_ptr<Texture2D> ResourceManager::UploadTexture(const std::string& path, DecodedImage& image, std::string& error)
{
	auto& state = GetState();
	auto replacement = Texture2D::Create(image.GetPixels(), image.GetWidth(), image.GetHeight(), image.GetChannels());
	if (!replacement || !replacement->IsLoaded()) { error = "Texture upload failed: " + path; return nullptr; }
	replacement->SetSourcePath(Utf8ToPath(path));
	const auto existing = state.m_named_textures.find(path);
	if (existing != state.m_named_textures.end())
	{
		if (!existing->second->SwapStorage(*replacement)) { error = "Incompatible texture replacement"; return nullptr; }
		replacement = existing->second;
	}
	else state.m_named_textures.emplace(path, replacement);
	++state.m_revision;
	return replacement;
}

std::shared_ptr<Model> ResourceManager::UploadModel(const std::string& key, ImportedModel& imported, std::string& error)
{
	auto& state = GetState();
	// Constructors are private; this is the designated resource factory.
	auto replacement = std::shared_ptr<Model>(new Model());
	std::map<std::string, std::shared_ptr<Texture2D>> textures;
	for (const auto& source : imported.materials)
		for (const auto& binding : source.textures)
		{
			if (textures.contains(binding.virtual_path)) continue;
			auto texture = Texture2D::Create(binding.image->GetPixels(), binding.image->GetWidth(), binding.image->GetHeight(), binding.image->GetChannels());
			if (!texture || !texture->IsLoaded()) { error = "Model texture upload failed"; return nullptr; }
			texture->SetSourcePath(Utf8ToPath(binding.virtual_path));
			textures.emplace(binding.virtual_path, std::move(texture));
		}
	for (const auto& source : imported.materials)
	{
		auto material = std::shared_ptr<Material>(new Material(source.name, source.ambient, source.diffuse, source.specular, source.shininess));
		for (const auto& binding : source.textures)
		{
			const auto existing = state.m_named_textures.find(binding.virtual_path);
			auto texture = existing == state.m_named_textures.end() ? textures.at(binding.virtual_path) : existing->second;
			switch (binding.slot)
			{
			case MaterialTextureSlot::Diffuse: material->m_map_diffuse = texture; break;
			case MaterialTextureSlot::Specular: material->m_map_specular = texture; break;
			case MaterialTextureSlot::Normal: material->m_map_normal = texture; break;
			case MaterialTextureSlot::Height: material->m_map_height = texture; break;
			}
		}
		replacement->m_material_data.push_back(std::move(material));
	}
	for (const auto& source : imported.meshes)
	{
		if (source.material_index >= replacement->m_material_data.size()) { error = "Invalid imported mesh material"; return nullptr; }
		replacement->m_meshes.push_back(std::shared_ptr<Mesh>(new Mesh(source.name, source.vertices, source.indices, source.material_index)));
	}
	if (replacement->m_meshes.empty()) { error = "No imported model geometry"; return nullptr; }
	replacement->ComputeBoundingSphere(); replacement->ComputeBoundingAABB();
	std::vector<std::string> swapped;
	for (auto& [path, texture] : textures)
	{
		const auto existing = state.m_named_textures.find(path);
		if (existing == state.m_named_textures.end()) continue;
		if (!existing->second->SwapStorage(*texture))
		{
			for (const auto& previous : swapped) (void)state.m_named_textures.at(previous)->SwapStorage(*textures.at(previous));
			error = "Incompatible model texture replacement";
			return nullptr;
		}
		swapped.push_back(path);
	}
	for (auto& [path, texture] : textures)
	{
		state.m_named_textures.try_emplace(path, texture);
		++state.m_texture_generations[path];
		if (path.find("#embedded-") == std::string::npos) state.m_texture_sources[path] = path;
	}
	for (size_t index = 0; index < replacement->m_material_data.size(); ++index)
		state.m_material[key + "|material|" + std::to_string(index)] = replacement->m_material_data[index];
	for (const auto& mesh : replacement->m_meshes)
	{
		state.m_meshes.insert(mesh);
		state.m_vertex_count += static_cast<int>(mesh->GetVertexCount());
	}
	const auto existing = state.m_models.find(key);
	if (existing != state.m_models.end())
	{
		*existing->second = std::move(*replacement);
		replacement = existing->second;
	}
	else state.m_models.emplace(key, replacement);
	++state.m_revision;
	return replacement;
}

JobHandle ResourceManager::LoadTextureAsync(std::string_view path, std::function<void(ResourceLoadResult<Texture2D>)> callback)
{
	auto& state = GetState();
	auto* jobs = s_instance->m_jobs;
	if (jobs && !jobs->IsAcceptingJobs()) return jobs->Submit([] {});
	const auto normalized = VirtualFileSystem::Normalize(path);
	const std::string key = normalized.value_or(std::string(path));
	const auto generation = ++state.m_texture_generations[key];
	auto* assets = s_instance->m_assets;
	const auto files = assets ? assets->GetFiles() : VirtualFileSystem();
	const auto snapshot = assets ? assets->GetRegistry() : AssetRegistry();
	auto image = std::make_shared<std::shared_ptr<DecodedImage>>();
	auto error = std::make_shared<std::string>();
	if (!assets || !normalized) *error = "No asset pipeline or invalid texture reference";
	auto decode = [files, key, image, error] { if (error->empty()) *image = AssetImporter::ImportTexture(files, key, *error); };
	auto upload = [key, image, error, snapshot, generation, callback = std::move(callback)]() mutable
	{
		auto& current = GetState();
		ResourceLoadResult<Texture2D> result;
		if (current.m_texture_generations[key] != generation ||
			(s_instance->m_assets && !Unchanged(snapshot, s_instance->m_assets->GetRegistry(), { key }))) result.error = "Superseded texture import";
		else if (!error->empty()) result.error = *error;
		else if (*image) result.resource = UploadTexture(key, **image, result.error);
		if (result.resource) current.m_texture_sources[key] = key;
		if (callback) callback(std::move(result));
	};
	if (!jobs) { decode(); upload(); return {}; }
	const std::array dependencies{ jobs->Submit(std::move(decode)) };
	return jobs->Submit(std::move(upload), dependencies, JobAffinity::MainThread);
}

JobHandle ResourceManager::LoadModelAsync(std::string_view path, std::function<void(ResourceLoadResult<Model>)> callback, ModelParserFlags flags)
{
	auto& state = GetState();
	auto* jobs = s_instance->m_jobs;
	if (jobs && !jobs->IsAcceptingJobs()) return jobs->Submit([] {});
	const auto normalized = VirtualFileSystem::Normalize(path);
	const std::string source = normalized.value_or(std::string(path));
	const auto key = source + "|" + std::to_string(static_cast<uint32_t>(flags));
	auto& entry = state.m_model_sources[key];
	entry.source = source; entry.flags = flags;
	const auto generation = ++entry.generation;
	auto* assets = s_instance->m_assets;
	const auto files = assets ? assets->GetFiles() : VirtualFileSystem();
	const auto snapshot = assets ? assets->GetRegistry() : AssetRegistry();
	auto imported = std::make_shared<ImportedModel>();
	auto error = std::make_shared<std::string>();
	if (!assets || !normalized) *error = "No asset pipeline or invalid model reference";
	auto parse = [files, source, imported, error, flags]
	{
		if (error->empty()) (void)AssetImporter::ImportModel(files, source, *imported, *error, flags);
	};
	auto upload = [key, source, imported, error, generation, snapshot, callback = std::move(callback)]() mutable
	{
		auto& current = GetState();
		ResourceLoadResult<Model> result;
		if (current.m_model_sources[key].generation != generation ||
			(s_instance->m_assets && !Unchanged(snapshot, s_instance->m_assets->GetRegistry(), imported->dependencies))) result.error = "Superseded model import";
		else if (!error->empty()) result.error = *error;
		else result.resource = UploadModel(key, *imported, result.error);
		if (result.resource) current.m_model_sources[key].dependencies = imported->dependencies;
		if (callback) callback(std::move(result));
	};
	if (!jobs) { parse(); upload(); return {}; }
	const std::array dependencies{ jobs->Submit(std::move(parse)) };
	return jobs->Submit(std::move(upload), dependencies, JobAffinity::MainThread);
}

JobHandle ResourceManager::LoadTextureAsync(AssetId id, std::function<void(ResourceLoadResult<Texture2D>)> callback)
{
	GetState();
	const auto* entry = s_instance->m_assets ? s_instance->m_assets->GetRegistry().Find(id) : nullptr;
	return LoadTextureAsync(entry ? entry->virtual_path : std::string(), std::move(callback));
}

JobHandle ResourceManager::LoadModelAsync(AssetId id, std::function<void(ResourceLoadResult<Model>)> callback, ModelParserFlags flags)
{
	GetState();
	const auto* entry = s_instance->m_assets ? s_instance->m_assets->GetRegistry().Find(id) : nullptr;
	return LoadModelAsync(entry ? entry->virtual_path : std::string(), std::move(callback), flags);
}

std::shared_ptr<Texture2D> ResourceManager::LoadTextureAsset(std::string_view path)
{
	auto& state = GetState();
	const auto normalized = VirtualFileSystem::Normalize(path);
	if (normalized)
	{
		const auto found = state.m_named_textures.find(*normalized);
		if (found != state.m_named_textures.end()) { ++state.m_texture_cache.hits; return found->second; }
	}
	ResourceLoadResult<Texture2D> result;
	const auto job = LoadTextureAsync(path, [&](auto loaded) { result = std::move(loaded); });
	if (s_instance->m_jobs && !s_instance->m_jobs->Wait(job)) return nullptr;
	if (!result.Succeeded()) { Log::Write(Log::Levels::Error, Log::Categories::Renderer, "{}", result.error); return nullptr; }
	++state.m_texture_cache.misses;
	return result.resource;
}

std::shared_ptr<Model> ResourceManager::LoadModelAsset(std::string_view path, ModelParserFlags flags)
{
	auto& state = GetState();
	const auto normalized = VirtualFileSystem::Normalize(path);
	const auto key = normalized.value_or(std::string(path)) + "|" + std::to_string(static_cast<uint32_t>(flags));
	const auto found = state.m_models.find(key);
	if (found != state.m_models.end()) { ++state.m_model_cache.hits; return found->second; }
	ResourceLoadResult<Model> result;
	const auto job = LoadModelAsync(path, [&](auto loaded) { result = std::move(loaded); }, flags);
	if (s_instance->m_jobs && !s_instance->m_jobs->Wait(job)) return nullptr;
	if (!result.Succeeded()) { Log::Write(Log::Levels::Error, Log::Categories::Renderer, "{}", result.error); return nullptr; }
	++state.m_model_cache.misses;
	return result.resource;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTextureAsset(AssetId id)
{
	GetState();
	const auto* entry = s_instance->m_assets ? s_instance->m_assets->GetRegistry().Find(id) : nullptr;
	return entry ? LoadTextureAsset(entry->virtual_path) : nullptr;
}

std::shared_ptr<Model> ResourceManager::LoadModelAsset(AssetId id, ModelParserFlags flags)
{
	GetState();
	const auto* entry = s_instance->m_assets ? s_instance->m_assets->GetRegistry().Find(id) : nullptr;
	return entry ? LoadModelAsset(entry->virtual_path, flags) : nullptr;
}

void ResourceManager::ReloadAssets(const std::vector<std::string>& changed)
{
	auto& state = GetState();
	std::vector<std::string> textures;
	std::vector<std::pair<std::string, ModelParserFlags>> models;
	std::vector<std::vector<std::string>> cubemaps;
	for (const auto& [key, source] : state.m_texture_sources)
		if (state.m_named_textures.contains(key) && std::find(changed.begin(), changed.end(), source) != changed.end()) textures.push_back(source);
	for (const auto& [key, source] : state.m_model_sources)
		if (state.m_models.contains(key) && std::any_of(source.dependencies.begin(), source.dependencies.end(), [&](const auto& dependency)
			{ return std::find(changed.begin(), changed.end(), dependency) != changed.end(); })) models.emplace_back(source.source, source.flags);
	for (const auto& path : textures)
		(void)LoadTextureAsync(path, [](auto result) { if (!result.Succeeded()) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Texture reload: {}", result.error); });
	for (const auto& [path, flags] : models)
		(void)LoadModelAsync(path, [](auto result) { if (!result.Succeeded()) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Model reload: {}", result.error); }, flags);
	for (const auto& [key, source] : state.m_cubemap_sources)
		if (!source.target.expired() && std::any_of(source.faces.begin(), source.faces.end(), [&](const auto& face)
			{ return std::find(changed.begin(), changed.end(), face) != changed.end(); })) cubemaps.push_back(source.faces);
	for (const auto& faces : cubemaps)
		(void)LoadCubemapAsync(faces, [](auto result) { if (!result.Succeeded()) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Cubemap reload: {}", result.error); });
}
