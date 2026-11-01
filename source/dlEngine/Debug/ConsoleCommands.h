#pragma once

#include "Macros.h"
#include <string>
#include <string_view>
#include <vector>

namespace Daedalus
{
	class DAEDALUS_API ConsoleCommands
	{
	public:
		[[nodiscard]] static bool ExecuteHelp(std::string_view command, std::vector<std::string>& output);
		[[nodiscard]] static std::vector<std::string> Complete(std::string_view prefix);
	};
}
