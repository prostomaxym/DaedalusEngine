#include "dlpch.h"
#include "AssetRegistry.h"
#include <fstream>
#include <limits>
#include <random>
#include <set>

namespace Daedalus
{
	namespace
	{
		bool ReadIdentity(const std::filesystem::path& source, AssetId& id)
		{
			auto metadata_path = source;
			metadata_path += ".meta";
			std::error_code ec;
			const bool exists = std::filesystem::exists(metadata_path, ec);
			if (ec) return false;
			if (!exists)
			{
				thread_local std::mt19937_64 generator(std::random_device{}());
				do { id = generator(); } while (id == 0);
				auto temporary_path = metadata_path;
				temporary_path += "." + std::to_string(id) + ".meta";
				std::ofstream output(temporary_path, std::ios::binary);
				output << "DaedalusAsset 1\n" << id << '\n';
				output.close();
				if (output) std::filesystem::rename(temporary_path, metadata_path, ec);
				if (!output || ec)
				{
					std::filesystem::remove(temporary_path, ec);
					return false;
				}
			}
			std::ifstream input(metadata_path, std::ios::binary);
			std::string signature, identity, extra;
			if (!std::getline(input, signature) || !std::getline(input, identity)) return false;
			if (!signature.empty() && signature.back() == '\r') signature.pop_back();
			if (!identity.empty() && identity.back() == '\r') identity.pop_back();
			if (signature != "DaedalusAsset 1") return false;
			id = 0;
			if (identity.empty()) return false;
			for (const char digit : identity)
			{
				if (digit < '0' || digit > '9' || id > ((std::numeric_limits<AssetId>::max)() - static_cast<unsigned int>(digit - '0')) / 10) return false;
				id = id * 10 + static_cast<unsigned int>(digit - '0');
			}
			return id != 0 && !(input >> extra) && input.eof();
		}
	}

	bool AssetRegistry::Scan(const VirtualFileSystem& files)
	{
		m_assets.clear();
		m_ids.clear();
		m_errors.clear();
		std::set<AssetId> duplicates;
		for (const auto& [mount, root] : files.GetMounts())
		{
			if (mount != "assets" && mount != "shaders") continue;
			std::string error;
			const auto paths = files.List(mount, error);
			if (!error.empty()) m_errors.push_back(mount + ": " + error);
			for (const auto& path : paths)
			{
				const auto source = files.Resolve(path);
				if (!source) continue;
				AssetMetadata metadata;
				metadata.virtual_path = path;
				metadata.source_path = *source;
				std::error_code ec;
				metadata.write_time = std::filesystem::last_write_time(*source, ec);
				if (!ec) metadata.size = std::filesystem::file_size(*source, ec);
				if (ec || !files.Resolve(path + ".meta") || !ReadIdentity(*source, metadata.id))
				{
					m_errors.push_back(path + ": cannot read/create valid asset metadata");
					continue;
				}
				const auto existing = m_ids.find(metadata.id);
				if (existing != m_ids.end() || duplicates.contains(metadata.id))
				{
					if (existing != m_ids.end())
					{
						m_errors.push_back(existing->second + ": duplicate asset UUID");
						m_assets.erase(existing->second);
						m_ids.erase(existing);
					}
					duplicates.insert(metadata.id);
					m_errors.push_back(path + ": duplicate asset UUID");
					continue;
				}
				m_ids.emplace(metadata.id, path);
				m_assets.emplace(path, std::move(metadata));
			}
		}
		return m_errors.empty();
	}

	const AssetMetadata* AssetRegistry::Find(AssetId id) const
	{
		const auto it = m_ids.find(id);
		return it == m_ids.end() ? nullptr : Find(it->second);
	}

	const AssetMetadata* AssetRegistry::Find(const std::string& virtual_path) const
	{
		const auto it = m_assets.find(virtual_path);
		return it == m_assets.end() ? nullptr : &it->second;
	}
}
