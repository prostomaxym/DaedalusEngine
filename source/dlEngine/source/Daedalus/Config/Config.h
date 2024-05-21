#pragma once

#include <filesystem>
#include <vector>
#include <unordered_map>

namespace Daedalus
{
	class Config
	{
	public:
		virtual void Load(const std::filesystem::path& path) = 0;
		virtual void Save(const std::filesystem::path& path) = 0;
		virtual void LoadDefault() = 0;
	};
}