#include "dlpch.h"
#include "OpenGLShaderLibrary.h"

#include "OpenGLShader.h"

#include <algorithm>
#include <filesystem>

using namespace Daedalus;

namespace
{
	constexpr auto cache_dir = "cache";
}

OpenGLShaderLibrary::OpenGLShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	DL_CORE_INFO("Loading shader library;");
	const auto test1 = path;
	const auto test = path / cache_dir;
	if (!std::filesystem::exists(path / cache_dir))
	{
		if(!std::filesystem::create_directory(path / cache_dir))
			DL_CORE_ERROR("Failed to create cached shaders directory");
	}

	const auto source_filenames = QueryFileNames(path);
	const auto shader_names = FilterSourceFileNames(source_filenames);
	const auto binary_shaders = QueryFileNames(path / cache_dir, true);
	const auto ready_to_load_shaders = FindOverlap(shader_names, binary_shaders);

	if (recompile)
	{
		RecompileAllShaders(shader_names, path);
	}
	else
	{
		LoadCachedShaders(ready_to_load_shaders, path);
		LoadNotCachedShaders(shader_names, ready_to_load_shaders, path);
	}

	DL_CORE_INFO("Shader library loaded successfully;");
}

void OpenGLShaderLibrary::RecompileAllShaders(const std::set<std::string>& shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : shader_names)
	{
		std::shared_ptr<OpenGLShader> shader;
		shader = std::make_shared<OpenGLShader>(path.string() + "/" + shader_name + ".vert", path.string() + "/" + shader_name + ".frag");
		shader->SaveBinary(path.string() + "/" + cache_dir + "/" + shader_name + ".dlshader");
		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
	}
}

void OpenGLShaderLibrary::LoadCachedShaders(const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : cached_shader_names)
	{
		std::shared_ptr<OpenGLShader> shader;
		try
		{
			shader = std::make_shared<OpenGLShader>(path.string() + "/" + cache_dir +"/" + shader_name + ".dlshader");
		}
		catch (std::runtime_error& er)
		{
			shader = std::make_shared<OpenGLShader>(path.string() + "/" + shader_name + ".vert", path.string() + "/" + shader_name + ".frag");
			shader->SaveBinary(path.string() + "/" +cache_dir + "/" + shader_name + ".dlshader");
		}

		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
	}
}

void OpenGLShaderLibrary::LoadNotCachedShaders(const std::set<std::string>& all_shader_names, 
	const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : all_shader_names)
	{
		if (std::find(cached_shader_names.begin(), cached_shader_names.end(), shader_name) == cached_shader_names.end())
		{
			std::shared_ptr<OpenGLShader> shader;
			shader = std::make_shared<OpenGLShader>(path.string() + "/" + shader_name + ".vert", path.string() + "/" + shader_name + ".frag");
			shader->SaveBinary(path.string() + "/" + cache_dir +"/" + shader_name + ".dlshader");
			shader->SetName(shader_name);
			m_shaders.insert({ shader_name, shader });
		}
	}
}

std::set<std::string> OpenGLShaderLibrary::QueryFileNames(const std::filesystem::path& path, bool ignore_extention) const
{
	std::set <std::string> file_names;

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if(entry.is_regular_file())
			file_names.insert(ignore_extention ? entry.path().stem().string() : entry.path().filename().string());
	}

	return file_names;
}

std::set<std::string> OpenGLShaderLibrary::FilterSourceFileNames(const std::set<std::string>& file_names) const
{
	// Parse all filenames for shaders pairs, removing shader files without pair

	std::set<std::string> shader_names;
	for (auto& filename : file_names)
	{
		const std::string delimiter = ".";
		std::string shader_name = filename.substr(0, filename.find(delimiter));

		auto pair = std::find_if(file_names.begin(), file_names.end(), [&shader_name, &delimiter, &filename](auto& str)
			{
				std::string pair_shadername = str.substr(0, str.find(delimiter));
				return (pair_shadername.find(shader_name) != std::string::npos) && str != filename;
			});

		if (pair != file_names.end())
			shader_names.insert(shader_name);
	}

	return shader_names;
}

std::vector<std::string> OpenGLShaderLibrary::FindOverlap(const std::set<std::string>& lhs, const std::set<std::string>& rhs) const
{
	// Load overlap
	std::vector<std::string> overlap;
	std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(overlap));

	return overlap;
}