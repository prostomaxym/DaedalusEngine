#pragma once

#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"

#include <set>

namespace Daedalus {

	class DAEDALUS_API OpenGLShaderLibrary final : public ShaderLibrary
	{
	public:
		OpenGLShaderLibrary(const std::filesystem::path& path, bool recompile);

	private:
		void RecompileAllShaders(const std::set<std::string>& shader_names, const std::filesystem::path& path);
		void LoadCachedShaders(const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path);
		void LoadNotCachedShaders(const std::set<std::string>& all_shader_names, const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path);

		std::vector<std::string> FindOverlap(const std::set<std::string>& lhs, const std::set<std::string>& rhs) const;
		std::set<std::string> QueryFileNames(const std::filesystem::path& path, bool ignore_extention = false) const;
		std::set<std::string> FilterSourceFileNames(const std::set<std::string>& file_names) const;
	};

}