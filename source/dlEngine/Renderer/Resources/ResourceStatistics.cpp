#include "dlpch.h"

#include "ResourceState.h"

using namespace Daedalus;

namespace
{
	template<typename T>
	long ExternalRefs(const std::shared_ptr<T>& ptr)
	{
		// The manager itself holds one reference
		return std::max(0L, static_cast<long>(ptr.use_count()) - 1L);
	}
}

double ResourceManager::GetLoadTime(const void* resource)
{
	const auto it = GetState().m_load_times.find(resource);
	return it != GetState().m_load_times.end() ? it->second : 0.0;
}

ResourceStats ResourceManager::GetStats()
{
	// Totals only change when a resource is added or removed, so rebuild them lazily
	if (!GetState().m_cached_stats_valid || GetState().m_cached_revision != GetState().m_revision)
	{
		ResourceStats stats;
		stats.models = GetState().m_models.size();
		stats.materials = GetState().m_material.size();
		stats.meshes = GetState().m_meshes.size();
		stats.named_textures = GetState().m_named_textures.size();
		stats.unnamed_textures = GetState().m_unnamed_textures.size();
		stats.unnamed_cubemaps = GetState().m_unnamed_cubemaps.size();
		stats.vertices = GetState().m_vertex_count;
		stats.shaders = GetState().m_shader_library ? GetState().m_shader_library->Count() : 0;

		for (const auto& mesh : GetState().m_meshes)
		{
			stats.indices += mesh->GetIndexCount();
			stats.vertex_buffer_bytes += mesh->GetVertexBufferBytes();
			stats.index_buffer_bytes += mesh->GetIndexBufferBytes();
		}
		stats.triangles = stats.indices / 3;

		for (const auto& [name, texture] : GetState().m_named_textures)
		{
			if (texture)
				stats.texture_bytes += texture->GetMemorySizeBytes();
		}
		for (const auto& texture : GetState().m_unnamed_textures)
		{
			if (texture)
				stats.texture_bytes += texture->GetMemorySizeBytes();
		}
		for (const auto& cubemap : GetState().m_unnamed_cubemaps)
		{
			if (cubemap)
				stats.cubemap_bytes += cubemap->GetMemorySizeBytes();
		}

		GetState().m_cached_stats = stats;
		GetState().m_cached_revision = GetState().m_revision;
		GetState().m_cached_stats_valid = true;
	}

	auto stats = GetState().m_cached_stats;
	stats.model_cache = GetState().m_model_cache;
	stats.material_cache = GetState().m_material_cache;
	stats.texture_cache = GetState().m_texture_cache;
	return stats;
}

std::vector<ShaderStats> ResourceManager::GetShaderStats()
{
	std::vector<ShaderStats> result;
	if (!GetState().m_shader_library)
		return result;

	result.reserve(GetState().m_shader_library->Count());
	for (const auto& [name, shader] : GetState().m_shader_library->GetShaders())
	{
		ShaderStats entry;
		entry.name = name;
		entry.info = shader->GetInfo();
		if (const auto* load_info = GetState().m_shader_library->GetLoadInfo(name))
			entry.load = *load_info;

		result.push_back(std::move(entry));
	}

	std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
	return result;
}

std::vector<ModelStats> ResourceManager::GetModelStats()
{
	std::vector<ModelStats> result;
	result.reserve(GetState().m_models.size());

	for (const auto& [name, model] : GetState().m_models)
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
	std::vector<MeshStats> result;
	result.reserve(GetState().m_meshes.size());

	for (const auto& mesh : GetState().m_meshes)
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
	std::vector<MaterialStats> result;
	result.reserve(GetState().m_material.size());

	for (const auto& [name, material] : GetState().m_material)
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
	std::unordered_map<const Texture2D*, std::vector<std::string>> used_by;
	for (const auto& [name, material] : GetState().m_material)
	{
		const std::shared_ptr<Texture2D> maps[] = { material->GetDiffuseMap(), material->GetSpecularMap(), material->GetNormalMap(), material->GetHeightMap() };
		for (const auto& map : maps)
		{
			if (map)
				used_by[map.get()].push_back(name);
		}
	}

	std::vector<TextureStats> result;
	result.reserve(GetState().m_named_textures.size() + GetState().m_unnamed_textures.size());

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

	for (const auto& [name, texture] : GetState().m_named_textures)
	{
		if (texture)
			result.push_back(fill(texture, name, true));
	}
	for (const auto& texture : GetState().m_unnamed_textures)
	{
		if (texture)
			result.push_back(fill(texture, "<unnamed>", false));
	}

	return result;
}

std::vector<CubemapStats> ResourceManager::GetCubemapStats()
{
	std::vector<CubemapStats> result;
	result.reserve(GetState().m_unnamed_cubemaps.size());

	for (const auto& cubemap : GetState().m_unnamed_cubemaps)
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
