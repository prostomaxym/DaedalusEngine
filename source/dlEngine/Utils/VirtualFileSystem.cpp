#include "dlpch.h"
#include "VirtualFileSystem.h"
#include "Utf8.h"
#include <algorithm>

namespace Daedalus
{
	bool VirtualFileSystem::Mount(std::string name, const std::filesystem::path& directory)
	{
		if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos || directory.empty())
			return false;
		std::error_code ec;
		const auto absolute = std::filesystem::absolute(directory, ec);
		if (ec) return false;
		const auto root = std::filesystem::weakly_canonical(absolute, ec);
		if (ec) return false;
		m_mounts[std::move(name)] = root;
		return true;
	}

	std::optional<std::filesystem::path> VirtualFileSystem::Resolve(std::string_view virtual_path) const
	{
		const auto separator = virtual_path.find(":/");
		if (separator == std::string_view::npos) return std::nullopt;
		const auto mount = m_mounts.find(std::string(virtual_path.substr(0, separator)));
		if (mount == m_mounts.end()) return std::nullopt;
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
}
