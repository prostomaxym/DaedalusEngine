#pragma once

#include "Macros.h"
#include <filesystem>
#include <functional>
#include <ostream>
#include <string>

namespace Daedalus
{
	class DAEDALUS_API BundledFileInstaller
	{
	public:
		using Writer = std::function<bool(std::ostream&)>;
		[[nodiscard]] static bool InstallMissing(const std::filesystem::path& root,
			const std::string& relative_path, const Writer& writer, std::string& error);
	};
}
