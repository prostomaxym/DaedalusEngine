#include "dlpch.h"
#include "VirtualFileSystem.h"
#include "Utf8.h"
#include <algorithm>
#include <fstream>
#include <zip.h>

namespace Daedalus
{
	namespace
	{
		using Archive = std::unique_ptr<zip_t, decltype(&zip_discard)>;
		Archive OpenArchive(const std::filesystem::path& path, std::string& error)
		{
			int code = 0;
			Archive archive(zip_open(Utf8ToString(path.u8string()).c_str(), ZIP_RDONLY | ZIP_CHECKCONS, &code), &zip_discard);
			if (!archive)
			{
				zip_error_t detail;
				zip_error_init_with_code(&detail, code);
				error = "Cannot open ZIP archive: " + std::string(zip_error_strerror(&detail)) + " (error " + std::to_string(code) + ")";
				zip_error_fini(&detail);
			}
			return archive;
		}

		bool ArchiveEntries(zip_t* archive, std::map<std::string, zip_uint64_t>& entries, std::string& error)
		{
			const auto count = zip_get_num_entries(archive, 0);
			if (count < 0 || count > 100000) { error = "ZIP entry count exceeds limit"; return false; }
			std::set<std::string> names;
			uint64_t total = 0;
			for (zip_int64_t index = 0; index < count; ++index)
			{
				zip_stat_t stat;
				zip_stat_init(&stat);
				if (zip_stat_index(archive, static_cast<zip_uint64_t>(index), 0, &stat) != 0 || !stat.name ||
					(stat.valid & (ZIP_STAT_SIZE | ZIP_STAT_ENCRYPTION_METHOD)) != (ZIP_STAT_SIZE | ZIP_STAT_ENCRYPTION_METHOD))
				{ error = "Invalid ZIP entry metadata"; return false; }
				std::string name(stat.name);
				// Compress-Archive on Windows can store backslashes; virtual references remain canonical.
				std::replace(name.begin(), name.end(), '\\', '/');
				const bool directory = !name.empty() && name.back() == '/';
				if (directory) name.pop_back();
				const auto normalized = VirtualFileSystem::Normalize("zip:/" + name);
				zip_uint8_t system = 0;
				zip_uint32_t attributes = 0;
				const bool symlink = zip_file_get_external_attributes(archive, static_cast<zip_uint64_t>(index), 0, &system, &attributes) == 0 &&
					system == ZIP_OPSYS_UNIX && ((attributes >> 16) & 0170000) == 0120000;
				if (!normalized || *normalized != "zip:/" + name || name.empty())
				{ error = "Unsafe ZIP entry path: " + std::string(stat.name); return false; }
				if (!names.insert(name).second)
				{ error = "Duplicate canonical ZIP entry: " + name; return false; }
				if (symlink || stat.encryption_method != ZIP_EM_NONE)
				{ error = "Symlink or encrypted ZIP entry: " + name; return false; }
				if (stat.size > VirtualFileSystem::MaxAssetBytes || stat.size > 1024ull * 1024 * 1024 - total)
				{ error = "ZIP size limit exceeded at entry: " + name; return false; }
				total += stat.size;
				if (!directory) entries.emplace(std::move(name), static_cast<zip_uint64_t>(index));
			}
			return true;
		}

		bool IsSourceEntry(const std::string& name)
		{
			const auto relative = std::filesystem::u8path(name);
			for (const auto& part : relative) if (part == "cache") return false;
			return relative.extension() != ".meta";
		}

		bool ReadArchiveEntry(zip_t* archive, const std::map<std::string, zip_uint64_t>& entries,
			const std::string& entry, std::string& data, std::string& error, size_t limit)
		{
			data.clear(); error.clear();
			limit = (std::min)(limit, VirtualFileSystem::MaxAssetBytes);
			const auto found = entries.find(entry);
			if (found == entries.end()) { error = "Missing ZIP asset: " + entry; return false; }
			zip_stat_t stat;
			zip_stat_init(&stat);
			if (zip_stat_index(archive, found->second, 0, &stat) != 0 || stat.size > limit || stat.encryption_method != ZIP_EM_NONE)
			{ error = "Missing, encrypted or oversized ZIP asset: " + entry; return false; }
			std::unique_ptr<zip_file_t, decltype(&zip_fclose)> input(zip_fopen_index(archive, found->second, 0), &zip_fclose);
			if (!input) { error = "Cannot open ZIP entry"; return false; }
			data.resize(static_cast<size_t>(stat.size));
			size_t read = 0;
			while (read < data.size())
			{
				const auto count = zip_fread(input.get(), data.data() + read, data.size() - read);
				if (count <= 0) { error = "Cannot read complete ZIP asset"; data.clear(); return false; }
				read += static_cast<size_t>(count);
			}
			char extra;
			if (zip_fread(input.get(), &extra, 1) != 0) { error = "ZIP asset integrity check failed"; data.clear(); return false; }
			return true;
		}
	}

	std::optional<std::string> VirtualFileSystem::Virtualize(const std::filesystem::path& path) const
	{
		std::error_code ec;
		const auto canonical = std::filesystem::weakly_canonical(path, ec);
		if (ec) return std::nullopt;
		for (const auto& [name, root] : m_mounts)
		{
			if (IsArchive(name)) continue;
			const auto relative = canonical.lexically_relative(root);
			if (relative.empty() || relative.is_absolute()) continue;
			const auto reference = Normalize(name + ":/" + Utf8ToString(relative.generic_u8string()));
			if (reference && Resolve(*reference) == canonical) return reference;
		}
		return std::nullopt;
	}

	bool VirtualFileSystem::Mount(std::string name, const std::filesystem::path& directory)
	{
		std::string error;
		return Mount(std::move(name), directory, error);
	}

	bool VirtualFileSystem::Mount(std::string name, const std::filesystem::path& directory, std::string& error)
	{
		error.clear();
		if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos || directory.empty())
		{ error = "Invalid mount name or empty source path"; return false; }
		std::error_code ec;
		const auto absolute = std::filesystem::absolute(directory, ec);
		if (ec) { error = ec.message(); return false; }
		const auto root = std::filesystem::weakly_canonical(absolute, ec);
		if (ec) { error = ec.message(); return false; }
		if (root.extension() == ".zip")
			return MountArchive(std::move(name), root, error);
		m_archives.erase(name);
		m_mounts[std::move(name)] = root;
		return true;
	}

	bool VirtualFileSystem::MountArchive(std::string name, const std::filesystem::path& path, std::string& error)
	{
		error.clear();
		if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
		{ error = "Invalid mount name"; return false; }
		std::error_code ec;
		const auto absolute = std::filesystem::absolute(path, ec);
		if (ec) { error = ec.message(); return false; }
		auto archive = OpenArchive(absolute, error);
		std::map<std::string, zip_uint64_t> entries;
		if (!archive || !ArchiveEntries(archive.get(), entries, error)) return false;
		m_mounts[name] = absolute;
		m_archives.insert(std::move(name));
		return true;
	}

	bool VirtualFileSystem::IsArchive(std::string_view mount) const { return m_archives.contains(std::string(mount)); }

	std::optional<std::string> VirtualFileSystem::Normalize(std::string_view path)
	{
		const auto separator = path.find(":/");
		if (separator == std::string_view::npos || separator == 0 ||
			path.substr(0, separator).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string_view::npos) return std::nullopt;
		const auto tail = path.substr(separator + 2);
		if (tail.find_first_of("\\:\0", 0, 3) != std::string_view::npos || (!tail.empty() && tail.front() == '/')) return std::nullopt;
		const auto relative = std::filesystem::u8path(tail.begin(), tail.end());
		for (const auto& part : relative) if (part == "..") return std::nullopt;
		return std::string(path.substr(0, separator + 2)) + (tail.empty() ? std::string() : Utf8ToString(relative.lexically_normal().generic_u8string()));
	}

	std::optional<std::string> VirtualFileSystem::Join(std::string_view source, std::string_view relative)
	{
		if (relative.find(":/") != std::string_view::npos) return Normalize(relative);
		const auto base = Normalize(source);
		if (!base || relative.empty()) return std::nullopt;
		std::string child(relative);
		std::replace(child.begin(), child.end(), '\\', '/');
		if (child.front() == '/' || child.find(':') != std::string::npos || child.find('\0') != std::string::npos) return std::nullopt;
		const auto separator = base->find(":/") + 2;
		const auto path = (std::filesystem::u8path(base->substr(separator)).parent_path() / std::filesystem::u8path(child)).lexically_normal();
		return Normalize(base->substr(0, separator) + Utf8ToString(path.generic_u8string()));
	}

	bool VirtualFileSystem::Read(std::string_view path, std::string& data, std::string& error, size_t limit) const
	{
		data.clear(); error.clear();
		const auto normalized = Normalize(path);
		if (!normalized) { error = "Invalid virtual path"; return false; }
		const auto separator = normalized->find(":/");
		const auto mount = m_mounts.find(normalized->substr(0, separator));
		if (mount == m_mounts.end()) { error = "Unknown mount"; return false; }
		limit = (std::min)(limit, MaxAssetBytes);
		if (IsArchive(mount->first))
		{
			auto archive = OpenArchive(mount->second, error);
			if (!archive) return false;
			// Resolve against this handle, not indices cached before an archive repack.
			std::map<std::string, zip_uint64_t> entries;
			if (!ArchiveEntries(archive.get(), entries, error)) return false;
			const auto entry = normalized->substr(separator + 2);
			return ReadArchiveEntry(archive.get(), entries, entry, data, error, limit);
		}
		const auto resolved = Resolve(*normalized);
		std::error_code ec;
		const auto size = resolved ? std::filesystem::file_size(*resolved, ec) : 0;
		if (!resolved || ec || size > limit) { error = "Missing, invalid or oversized asset source"; return false; }
		std::ifstream input(*resolved, std::ios::binary);
		data.resize(static_cast<size_t>(size));
		if (!input || !input.read(data.data(), static_cast<std::streamsize>(data.size()))) { error = "Cannot read complete asset source"; data.clear(); return false; }
		return true;
	}

	std::optional<std::filesystem::path> VirtualFileSystem::Resolve(std::string_view virtual_path) const
	{
		const auto normalized = Normalize(virtual_path);
		if (!normalized) return std::nullopt;
		virtual_path = *normalized;
		const auto separator = virtual_path.find(":/");
		if (separator == std::string_view::npos) return std::nullopt;
		const auto mount = m_mounts.find(std::string(virtual_path.substr(0, separator)));
		if (mount == m_mounts.end() || IsArchive(mount->first)) return std::nullopt;
		const auto tail = virtual_path.substr(separator + 2);
		if (tail.find_first_of("\\:\0", 0, 3) != std::string_view::npos) return std::nullopt;
		const auto relative = std::filesystem::u8path(tail.begin(), tail.end());
		if (relative.is_absolute() || relative.has_root_path()) return std::nullopt;
		for (const auto& part : relative)
			if (part == "..") return std::nullopt;
		std::error_code ec;
		const auto resolved = std::filesystem::weakly_canonical(mount->second / relative, ec);
		if (ec) return std::nullopt;
		const auto contained = resolved.lexically_relative(mount->second);
		if (contained.empty() || contained.is_absolute()) return std::nullopt;
		for (const auto& part : contained)
			if (part == "..") return std::nullopt;
		return resolved;
	}

	std::vector<std::string> VirtualFileSystem::List(std::string_view mount_name, std::string& error) const
	{
		error.clear();
		std::vector<std::string> files;
		const auto mount = m_mounts.find(std::string(mount_name));
		if (mount == m_mounts.end()) { error = "Unknown mount"; return files; }
		if (IsArchive(mount->first))
		{
			auto archive = OpenArchive(mount->second, error);
			std::map<std::string, zip_uint64_t> entries;
			if (!archive || !ArchiveEntries(archive.get(), entries, error)) return files;
			for (const auto& [entry, index] : entries)
			{
				if (IsSourceEntry(entry)) files.push_back(mount->first + ":/" + entry);
			}
			std::sort(files.begin(), files.end());
			return files;
		}
		std::error_code ec;
		if (!std::filesystem::exists(mount->second, ec))
		{
			if (ec) error = ec.message();
			return files;
		}
		std::filesystem::recursive_directory_iterator it(mount->second, ec), end;
		while (!ec && it != end)
		{
			const auto& entry = *it;
			if (entry.is_directory(ec) && entry.path().filename() == "cache")
				it.disable_recursion_pending();
			if (!ec && entry.is_regular_file(ec) && entry.path().extension() != ".meta")
			{
				const auto path = mount->first + ":/" + Utf8ToString(entry.path().lexically_relative(mount->second).generic_u8string());
				if (Resolve(path)) files.push_back(path);
			}
			if (!ec) it.increment(ec);
		}
		if (ec) error = ec.message();
		std::sort(files.begin(), files.end());
		return files;
	}

	bool VirtualFileSystem::VisitSources(std::string_view mount_name, const SourceVisitor& visitor, std::string& error) const
	{
		error.clear();
		const auto mount = m_mounts.find(std::string(mount_name));
		if (mount == m_mounts.end()) { error = "Unknown mount"; return false; }
		if (!visitor) { error = "Missing source visitor"; return false; }
		if (!IsArchive(mount->first))
		{
			const auto paths = List(mount_name, error);
			if (!error.empty()) return false;
			const ReadCallback reader = [this](std::string_view path, std::string& data, std::string& read_error, size_t limit)
			{
				return Read(path, data, read_error, limit);
			};
			for (const auto& path : paths) visitor(path, reader);
			return true;
		}
		auto archive = OpenArchive(mount->second, error);
		std::map<std::string, zip_uint64_t> entries;
		if (!archive || !ArchiveEntries(archive.get(), entries, error)) return false;
		const auto prefix = mount->first + ":/";
		const ReadCallback reader = [&](std::string_view path, std::string& data, std::string& read_error, size_t limit)
		{
			const auto normalized = Normalize(path);
			if (!normalized || normalized->rfind(prefix, 0) != 0)
			{ data.clear(); read_error = "Invalid path or mount for ZIP scan"; return false; }
			return ReadArchiveEntry(archive.get(), entries, normalized->substr(prefix.size()), data, read_error, limit);
		};
		for (const auto& [entry, index] : entries)
			if (IsSourceEntry(entry)) visitor(prefix + entry, reader);
		return true;
	}
}
