#pragma once

#include "ResourceManager.h"
#include "Renderer/Resourses/ShaderLibrary.h"
#include "Renderer/Resourses/Material.h"
#include "Renderer/Resourses/Mesh.h"
#include "Renderer/Resourses/Texture.h"
#include <set>
#include <thread>
#include <unordered_map>

namespace Daedalus
{
	struct ResourceManager::State
	{
		const std::thread::id m_owner{ std::this_thread::get_id() };
		std::unique_ptr<ShaderLibrary> m_shader_library;
		std::unordered_map<std::string, std::shared_ptr<Model>> m_models;
		std::unordered_map<std::string, std::shared_ptr<Material>> m_material;
		std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_named_textures;
		std::set<std::shared_ptr<Texture2D>> m_unnamed_textures;
		std::set<std::shared_ptr<TextureCubemap>> m_unnamed_cubemaps;
		std::set<std::shared_ptr<Mesh>> m_meshes;
		int m_vertex_count{ 0 };
		std::unordered_map<const void*, double> m_load_times;
		CacheCounters m_model_cache{};
		CacheCounters m_material_cache{};
		CacheCounters m_texture_cache{};
		uint64_t m_revision{ 0 };
		uint64_t m_cached_revision{ 0 };
		bool m_cached_stats_valid{ false };
		ResourceStats m_cached_stats{};
	};
}
