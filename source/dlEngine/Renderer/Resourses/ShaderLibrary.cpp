#include "dlpch.h"
#include "ShaderLibrary.h"

#include "Shader.h"

#include <algorithm>

using namespace Daedalus;

namespace
{
	constexpr auto cache_dir = "cache";
}

ShaderLibrary::ShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loading shader library;");
	if (!std::filesystem::exists(path / cache_dir))
	{
		if (!std::filesystem::create_directory(path / cache_dir))
			Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Failed to create cached shaders directory");
	}

	const auto source_filenames = QueryFileNames(path, Shader::GetSourceFileExtention(), true);
	const auto binary_shaders = QueryFileNames(path / cache_dir, Shader::GetBinaryFileExtention(), true);
	const auto ready_to_load_shaders = FindOverlap(source_filenames, binary_shaders);

	if (recompile)
	{
		RecompileAllShaders(source_filenames, path);
	}
	else
	{
		LoadCachedShaders(ready_to_load_shaders, path);
		LoadNotCachedShaders(source_filenames, ready_to_load_shaders, path);
	}

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shader library loaded successfully;");
}

void ShaderLibrary::RecompileAllShaders(const std::set<std::string>& shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : shader_names)
	{
		const auto shader_name_with_ext = shader_name + Shader::GetSourceFileExtention();
		auto shader = Shader::CreateFromFile(path / shader_name_with_ext);
		shader->SaveBinary(path.string() + "/" + cache_dir + "/" + shader_name + Shader::GetBinaryFileExtention());
		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
	}
}

void ShaderLibrary::LoadCachedShaders(const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : cached_shader_names)
	{
		std::shared_ptr<Shader> shader;
		try
		{
			shader = Shader::CreateFromBinary(path.string() + "/" + cache_dir + "/" + shader_name + Shader::GetBinaryFileExtention());
		}
		catch (std::runtime_error& er)
		{
			const auto shader_name_with_ext = shader_name + Shader::GetSourceFileExtention();
			shader = Shader::CreateFromFile(path / shader_name_with_ext);
			shader->SaveBinary(path.string() + "/" + cache_dir + "/" + shader_name + Shader::GetBinaryFileExtention());
		}

		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
	}
}

void ShaderLibrary::LoadNotCachedShaders(const std::set<std::string>& all_shader_names,
	const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (const auto& shader_name : all_shader_names)
	{
		if (!shader_name.empty() && std::find(cached_shader_names.begin(), cached_shader_names.end(), shader_name) == cached_shader_names.end())
		{
			const auto shader_name_with_ext = shader_name + Shader::GetSourceFileExtention();
			auto shader = Shader::CreateFromFile(path / shader_name_with_ext);
			shader->SaveBinary(path.string() + "/" + cache_dir + "/" + shader_name +Shader::GetBinaryFileExtention());
			shader->SetName(shader_name);
			m_shaders.insert({ shader_name, shader });
		}
	}
}

std::set<std::string> ShaderLibrary::QueryFileNames(const std::filesystem::path& path
	, const std::string& extention
	, bool exclude_extention) const
{
	std::set <std::string> file_names;

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file() && entry.path().extension() == extention)
			file_names.insert(exclude_extention ? entry.path().stem().string() : entry.path().filename().string());
	}

	return file_names;
}

std::vector<std::string> ShaderLibrary::FindOverlap(const std::set<std::string>& lhs, const std::set<std::string>& rhs) const
{
	// Load overlap
	std::vector<std::string> overlap;
	std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(overlap));

	return overlap;
}

void ShaderLibrary::Add(std::shared_ptr<Shader> shader)
{
	m_shaders.insert({ shader->GetName(), shader});
}
void ShaderLibrary::Remove(const std::string& name)
{
	m_shaders.erase(name);
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) const
{
	DL_ASSERT(Exists(name), Log::Categories::Renderer, "Cant find shader " + name + " in ShaderLibrary");
	return m_shaders.at(name);
}

bool ShaderLibrary::Exists(const std::string& name) const
{
	return m_shaders.find(name) != m_shaders.end();
}