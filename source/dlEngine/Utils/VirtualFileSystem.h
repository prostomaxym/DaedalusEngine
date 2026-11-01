#pragma once

#include "Macros.h"
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <set>

namespace Daedalus
{
	class DAEDALUS_API VirtualFileSystem
	{
	public:
		static constexpr size_t MaxAssetBytes = 256 * 1024 * 1024;
		using ReadCallback = std::function<bool(std::string_view, std::string&, std::string&, size_t)>;
		using SourceVisitor = std::function<void(const std::string&, const ReadCallback&)>;
		[[nodiscard]] bool Mount(std::string name, const std::filesystem::path& directory);
		[[nodiscard]] bool Mount(std::string name, const std::filesystem::path& directory, std::string& error);
		[[nodiscard]] bool MountArchive(std::string name, const std::filesystem::path& archive, std::string& error);
		[[nodiscard]] bool IsArchive(std::string_view mount) const;
		[[nodiscard]] static std::optional<std::string> Normalize(std::string_view virtual_path);
		[[nodiscard]] static std::optional<std::string> Join(std::string_view source, std::string_view relative);
		[[nodiscard]] std::optional<std::string> Virtualize(const std::filesystem::path& path) const;
		[[nodiscard]] bool Read(std::string_view virtual_path, std::string& data, std::string& error, size_t limit = MaxAssetBytes) const;
		[[nodiscard]] std::optional<std::filesystem::path> Resolve(std::string_view virtual_path) const;
		[[nodiscard]] std::vector<std::string> List(std::string_view mount, std::string& error) const;
		// Synchronous, scan-scoped reader; visitors must not retain it or use it concurrently.
		[[nodiscard]] bool VisitSources(std::string_view mount, const SourceVisitor& visitor, std::string& error) const;
		[[nodiscard]] const std::map<std::string, std::filesystem::path>& GetMounts() const { return m_mounts; }

	private:
		std::map<std::string, std::filesystem::path> m_mounts;
		std::set<std::string> m_archives;
	};
}
