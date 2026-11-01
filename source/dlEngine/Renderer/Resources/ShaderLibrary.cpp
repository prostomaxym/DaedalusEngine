#include "dlpch.h"
#include "ShaderLibrary.h"

#include "Shader.h"
#include "Core/AssetPipeline.h"
#include "Utils/AssetSourceReader.h"
#include "Utils/ContentFingerprint.h"
#include "Utils/Utf8.h"
#include <fstream>

#include <algorithm>

using namespace Daedalus;

namespace
{
	constexpr auto cache_dir = "cache";
}

ShaderLibrary::ShaderLibrary(const std::filesystem::path& path, bool recompile) : m_path(path)
{
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loading shader library;");
	std::error_code ec;
	m_archive = std::filesystem::is_regular_file(path, ec) && path.extension() == ".zip";
	if (m_archive)
	{
		VirtualFileSystem files;
		std::string error;
		if (!files.MountArchive("shaders", path, error)) throw std::runtime_error(error);
		const auto entries = files.List("shaders", error);
		if (!error.empty()) throw std::runtime_error(error);
		for (const auto& reference : entries)
		{
			const auto source = Utf8ToPath(reference.substr(9));
			if (source.has_parent_path() || source.extension() != Shader::GetSourceFileExtention()) continue;
			const auto name = source.stem().string();
			const auto start = std::chrono::steady_clock::now();
			auto shader = Shader::CreateFromCode(ReadSource(name));
			shader->SetName(name);
			m_shaders.emplace(name, shader);
			RecordLoadInfo(name, "compiled from ZIP source", start, path);
		}
		return;
	}
	bool has_source_time = false;
	std::filesystem::recursive_directory_iterator it(path, ec), end;
	while (!ec && it != end)
	{
		if (it->is_directory(ec) && it->path().filename() == cache_dir) it.disable_recursion_pending();
		if (!ec && it->is_regular_file(ec) && it->path().extension() != ".meta")
		{
			const auto time = it->last_write_time(ec);
			if (!ec && (!has_source_time || time > m_latest_source_time)) { m_latest_source_time = time; has_source_time = true; }
		}
		if (!ec) it.increment(ec);
	}
	if (ec) recompile = true;
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
		const auto start = std::chrono::steady_clock::now();
		const auto source = ReadSource(shader_name);
		auto shader = Shader::CreateFromCode(source);
		SaveCache(*shader, shader_name, source);
		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
		RecordLoadInfo(shader_name, "compiled from source (forced recompile)", start, path);
	}
}

void ShaderLibrary::LoadCachedShaders(const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (auto& shader_name : cached_shader_names)
	{
		const auto start = std::chrono::steady_clock::now();
		const char* origin = "binary cache";
		std::shared_ptr<Shader> shader;
		std::error_code ec;
		const auto binary_path = path / cache_dir / (shader_name + Shader::GetBinaryFileExtention());
		const auto binary_time = std::filesystem::last_write_time(binary_path, ec);
		const auto source = ReadSource(shader_name);
		std::ifstream fingerprint(binary_path.string() + ".hash");
		uint64_t cached_hash = 0;
		if (ec || binary_time < m_latest_source_time || !(fingerprint >> cached_hash) || cached_hash != ContentFingerprint(source))
		{
			shader = Shader::CreateFromCode(source);
			SaveCache(*shader, shader_name, source);
			shader->SetName(shader_name);
			m_shaders.emplace(shader_name, shader);
			RecordLoadInfo(shader_name, "compiled from source (stale binary cache)", start, path);
			continue;
		}
		try
		{
			shader = Shader::CreateFromBinary(path.string() + "/" + cache_dir + "/" + shader_name + Shader::GetBinaryFileExtention());
		}
		catch (std::runtime_error& er)
		{
			shader = Shader::CreateFromCode(source);
			SaveCache(*shader, shader_name, source);
			origin = "compiled from source (binary cache failed)";
		}

		shader->SetName(shader_name);
		m_shaders.insert({ shader_name, shader });
		RecordLoadInfo(shader_name, origin, start, path);
	}
}

void ShaderLibrary::LoadNotCachedShaders(const std::set<std::string>& all_shader_names,
	const std::vector<std::string>& cached_shader_names, const std::filesystem::path& path)
{
	for (const auto& shader_name : all_shader_names)
	{
		if (!shader_name.empty() && std::find(cached_shader_names.begin(), cached_shader_names.end(), shader_name) == cached_shader_names.end())
		{
			const auto start = std::chrono::steady_clock::now();
			const auto source = ReadSource(shader_name);
			auto shader = Shader::CreateFromCode(source);
			SaveCache(*shader, shader_name, source);
			shader->SetName(shader_name);
			m_shaders.insert({ shader_name, shader });
			RecordLoadInfo(shader_name, "compiled from source (not cached)", start, path);
		}
	}
}

void ShaderLibrary::RecordLoadInfo(const std::string& name, const char* origin, std::chrono::steady_clock::time_point start, const std::filesystem::path& path)
{
	ShaderLoadInfo info;
	info.origin = origin;
	info.load_time_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

	const auto source_file = path / (name + Shader::GetSourceFileExtention());
	info.source_path = m_archive ? "shaders:/" + name + Shader::GetSourceFileExtention() : source_file.string();

	std::error_code ec;
	const auto write_time = std::filesystem::last_write_time(m_archive ? path : source_file, ec);
	if (!ec)
	{
		// C++17 has no portable file_clock -> system_clock conversion
		info.last_modified = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			write_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		info.has_last_modified = true;
	}

	m_load_info[name] = std::move(info);
}

const ShaderLoadInfo* ShaderLibrary::GetLoadInfo(const std::string& name) const
{
	const auto it = m_load_info.find(name);
	return it != m_load_info.end() ? &it->second : nullptr;
}

void ShaderLibrary::ReloadAssets(AssetPipeline& assets, std::function<void()> on_reload)
{
	const auto generation = ++m_reload_generation;
	for (const auto& [path, metadata] : assets.GetRegistry().GetAssets())
	{
		if (path.rfind("shaders:/", 0) != 0 || Utf8ToPath(path.substr(9)).extension() != Shader::GetSourceFileExtention() ||
			path.find('/', 9) != std::string::npos) continue;
		const auto name = Utf8ToPath(path.substr(9)).stem().string();
		const auto start = std::chrono::steady_clock::now();
		const auto job = assets.LoadAsync(metadata.id, [this, generation, name, start, on_reload](AssetLoadResult result)
		{
			if (generation != m_reload_generation) return;
			if (!result.Succeeded())
			{
				Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Shader reload {}: {}", name, result.error);
				return;
			}
			const auto existing = m_shaders.find(name);
			std::shared_ptr<Shader> shader;
			if (existing != m_shaders.end())
			{
				shader = existing->second;
				if (!shader->ReloadFromCode(result.data)) return;
			}
			else
			{
				shader = Shader::TryCreateFromCode(result.data);
				if (!shader) return;
				shader->SetName(name);
				m_shaders.emplace(name, shader);
			}
			SaveCache(*shader, name, result.data);
			RecordLoadInfo(name, "hot reloaded from source", start, m_path);
			if (on_reload) on_reload();
			Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Hot reloaded shader {}", name);
		}, AssetLoadMode::ShaderSource);
		if (job.HasFailed()) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Shader reload submission rejected: {}", name);
	}
}


std::string ShaderLibrary::ReadSource(const std::string& name) const
{
	VirtualFileSystem files;
	std::string source, error;
	if (!files.Mount("shaders", m_path) || !AssetSourceReader::ReadShaderSource(files, "shaders:/" + name + Shader::GetSourceFileExtention(), source, error))
	{
		Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Startup shader source failed: {}", error);
		throw std::runtime_error(error);
	}
	return source;
}

void ShaderLibrary::SaveCache(const Shader& shader, const std::string& name, const std::string& source) const
{
	if (m_archive) return;
	const auto binary = m_path / cache_dir / (name + Shader::GetBinaryFileExtention());
	shader.SaveBinary(binary);
	auto fingerprint = binary;
	fingerprint += ".hash";
	std::ofstream output(fingerprint);
	output << ContentFingerprint(source) << '\n';
	if (!output) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Cannot save shader cache fingerprint: {}", name);
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