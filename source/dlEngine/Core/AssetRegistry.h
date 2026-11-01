#pragma once

#include "Macros.h"
#include "Utils/VirtualFileSystem.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Daedalus
{
	using AssetId = uint64_t;

	struct AssetMetadata
	{
		AssetId id = 0;
		std::string virtual_path;
		std::filesystem::path source_path;
		std::filesystem::file_time_type write_time{};
		uintmax_t size = 0;
		uint64_t content_hash = 0;
	};

	// Instances are owner-thread objects; background scans operate on independent copies.
	class DAEDALUS_API AssetRegistry
	{
	public:
		[[nodiscard]] bool Scan(const VirtualFileSystem& files);
		[[nodiscard]] const AssetMetadata* Find(AssetId id) const;
		[[nodiscard]] const AssetMetadata* Find(const std::string& virtual_path) const;
		[[nodiscard]] const std::map<std::string, AssetMetadata>& GetAssets() const { return m_assets; }
		[[nodiscard]] const std::vector<std::string>& GetErrors() const { return m_errors; }

	private:
		std::map<std::string, AssetMetadata> m_assets;
		std::map<AssetId, std::string> m_ids;
		std::vector<std::string> m_errors;
	};
}
