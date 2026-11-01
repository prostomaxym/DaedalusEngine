#pragma once

#include "Macros.h"
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Daedalus
{
	class DAEDALUS_API VirtualFileSystem
	{
	public:
		[[nodiscard]] bool Mount(std::string name, const std::filesystem::path& directory);
		[[nodiscard]] std::optional<std::filesystem::path> Resolve(std::string_view virtual_path) const;
		[[nodiscard]] std::vector<std::string> List(std::string_view mount, std::string& error) const;
		[[nodiscard]] const std::map<std::string, std::filesystem::path>& GetMounts() const { return m_mounts; }

	private:
		std::map<std::string, std::filesystem::path> m_mounts;
	};
}
