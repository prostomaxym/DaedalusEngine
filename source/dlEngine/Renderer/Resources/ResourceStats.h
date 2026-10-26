#pragma once

#include "Renderer/Resources/Shader.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace Daedalus
{
	struct CacheCounters
	{
		uint64_t hits{ 0 };
		uint64_t misses{ 0 };

		uint64_t Total() const { return hits + misses; }
		double HitRatePercent() const { return Total() == 0 ? 0.0 : 100.0 * static_cast<double>(hits) / static_cast<double>(Total()); }
	};

	struct ResourceStats
	{
		size_t models{ 0 };
		size_t materials{ 0 };
		size_t meshes{ 0 };
		size_t named_textures{ 0 };
		size_t unnamed_textures{ 0 };
		size_t unnamed_cubemaps{ 0 };
		size_t shaders{ 0 };
		int vertices{ 0 };
		uint64_t indices{ 0 };
		uint64_t triangles{ 0 };

		uint64_t texture_bytes{ 0 };
		uint64_t cubemap_bytes{ 0 };
		uint64_t vertex_buffer_bytes{ 0 };
		uint64_t index_buffer_bytes{ 0 };

		CacheCounters model_cache;
		CacheCounters material_cache;
		CacheCounters texture_cache;

		uint64_t TotalGpuBytes() const { return texture_bytes + cubemap_bytes + vertex_buffer_bytes + index_buffer_bytes; }
	};

	struct ModelStats
	{
		std::string name;
		std::vector<std::string> mesh_names;
		size_t meshes{ 0 };
		size_t materials{ 0 };
		uint64_t vertices{ 0 };
		uint64_t indices{ 0 };
		long ref_count{ 0 };
		double load_time_ms{ 0.0 };
	};

	struct MeshStats
	{
		std::string name;
		uint32_t vertices{ 0 };
		uint32_t indices{ 0 };
		uint32_t material_index{ 0 };
		uint64_t vertex_buffer_bytes{ 0 };
		uint64_t index_buffer_bytes{ 0 };
		float bounding_radius{ 0.f };
		std::array<float, 3> aabb_min{ 0.f, 0.f, 0.f };
		std::array<float, 3> aabb_max{ 0.f, 0.f, 0.f };
		long ref_count{ 0 };
	};

	struct MaterialTextureRef
	{
		const char* slot{ "" };
		uint32_t renderer_id{ 0 };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
	};

	struct MaterialStats
	{
		std::string name;
		std::array<float, 3> ambient{ 0.f, 0.f, 0.f };
		std::array<float, 3> diffuse{ 0.f, 0.f, 0.f };
		std::array<float, 3> specular{ 0.f, 0.f, 0.f };
		float shininess{ 0.f };
		std::vector<MaterialTextureRef> textures;
		long ref_count{ 0 };
	};

	struct TextureStats
	{
		std::string name;
		std::string path;
		std::string format;
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t mip_levels{ 1 };
		uint32_t renderer_id{ 0 };
		uint64_t bytes{ 0 };
		bool named{ false };
		long ref_count{ 0 };
		double load_time_ms{ 0.0 };
		std::vector<std::string> used_by;
	};

	struct CubemapStats
	{
		std::string format;
		uint32_t face_width{ 0 };
		uint32_t face_height{ 0 };
		uint32_t renderer_id{ 0 };
		uint64_t bytes{ 0 };
		long ref_count{ 0 };
		double load_time_ms{ 0.0 };
	};

	struct ShaderStats
	{
		std::string name;
		ShaderInfo info;
		ShaderLoadInfo load;
	};

	inline std::string FormatBytes(uint64_t bytes)
	{
		constexpr const char* units[] = { "B", "KB", "MB", "GB", "TB" };

		auto value = static_cast<double>(bytes);
		size_t unit = 0;
		while (value >= 1024.0 && unit < 4)
		{
			value /= 1024.0;
			unit++;
		}

		char buffer[32];
		if (unit == 0)
			std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(bytes));
		else
			std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);

		return buffer;
	}
}
