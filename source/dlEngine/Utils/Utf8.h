#pragma once

#include <filesystem>
#include <string>

namespace Daedalus
{
	[[nodiscard]] inline std::string Utf8ToString(const std::u8string& text)
	{
		// Keep UTF-8 bytes unchanged when crossing char8_t and existing char APIs.
		return { reinterpret_cast<const char*>(text.data()), text.size() };
	}

	[[nodiscard]] inline std::filesystem::path Utf8ToPath(const std::string& text)
	{
		// Replacement for std::filesystem::u8path, deprecated in C++20.
		return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
	}
}
