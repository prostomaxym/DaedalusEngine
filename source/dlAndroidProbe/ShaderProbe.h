#pragma once

#include <filesystem>

namespace Daedalus
{
	[[nodiscard]] bool CheckEngineShaders(const std::filesystem::path& directory);
}
