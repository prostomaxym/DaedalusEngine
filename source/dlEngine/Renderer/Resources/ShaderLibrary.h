#pragma once

#include "Shader.h"

#include <filesystem>
#include <set>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Daedalus
{
	class AssetPipeline;
	class DAEDALUS_API ShaderLibrary final
	{
	public:
		ShaderLibrary() = default;
		ShaderLibrary(const std::filesystem::path& path, bool recompile);

		void Add(std::shared_ptr<Shader> shader);
		void Remove(const std::string& name);

		std::shared_ptr<Shader> Get(const std::string& name) const;
		bool Exists(const std::string& name) const;
		size_t Count() const { return m_shaders.size(); }
		const std::unordered_map<std::string, std::shared_ptr<Shader>>& GetShaders() const { return m_shaders; }
		const ShaderLoadInfo* GetLoadInfo(const std::string& name) const;
		void ReloadAssets(AssetPipeline& assets, std::function<void()> on_reload);

	private:
		void RecordLoadInfo(const std::string& name, const char* origin, std::chrono::steady_clock::time_point start, const std::filesystem::path& path);
		void RecompileAllShaders(const std::set<std::string>& shader_names, const std::filesystem::path& path);
		void LoadCachedShaders(const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path);
		void LoadNotCachedShaders(const std::set<std::string>& all_shader_names, const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path);

		std::vector<std::string> FindOverlap(const std::set<std::string>& lhs, const std::set<std::string>& rhs) const;
		std::set<std::string> QueryFileNames(const std::filesystem::path& path, const std::string& extention, bool ignore_extention = false) const;

		std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
		std::unordered_map<std::string, ShaderLoadInfo> m_load_info;
		std::filesystem::path m_path;
		uint64_t m_reload_generation = 0;
		std::filesystem::file_time_type m_latest_source_time{};
	};
}