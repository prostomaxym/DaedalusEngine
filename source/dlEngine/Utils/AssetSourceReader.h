#pragma once

#include "Macros.h"
#include <filesystem>
#include <string>

namespace Daedalus
{
	class VirtualFileSystem;

	class DAEDALUS_API AssetSourceReader
	{
	public:
		[[nodiscard]] static bool ReadBytes(const std::filesystem::path& path, std::string& data, std::string& error);
		[[nodiscard]] static bool ReadShaderSource(const VirtualFileSystem& files, const std::string& virtual_path, std::string& data, std::string& error);
	};
}
