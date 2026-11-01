#pragma once

#include <cstdint>
#include <string_view>

namespace Daedalus
{
	inline uint64_t ContentFingerprint(std::string_view bytes)
	{
		uint64_t hash = 14695981039346656037ull;
		for (const unsigned char byte : bytes)
		{
			hash ^= byte;
			hash *= 1099511628211ull;
		}
		return hash;
	}
}
