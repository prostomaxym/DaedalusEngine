#pragma once

#include "Macros.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Daedalus
{
	class CVarRegistry;

	class DAEDALUS_API ConsoleCompletion
	{
	public:
		struct Matches
		{
			size_t offset{ 0 };
			size_t length{ 0 };
			std::vector<std::string> candidates;
		};

		[[nodiscard]] static Matches Find(std::string_view input, size_t cursor, const CVarRegistry& cvars);
		[[nodiscard]] static std::string Hint(std::string_view input, size_t cursor, bool has_selection,
			size_t buffer_capacity, const CVarRegistry& cvars);
	};
}
