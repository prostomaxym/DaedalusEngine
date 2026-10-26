#include <gtest/gtest.h>

#include "Utils/Utf8.h"

#include <filesystem>

namespace Daedalus
{
	TEST(Utf8Test, preserves_non_ascii_bytes)
	{
		EXPECT_EQ(Utf8ToString(u8"assets/\u00e9\u4e16\U0001f680.png"), "assets/\xc3\xa9\xe4\xb8\x96\xf0\x9f\x9a\x80.png");
	}

	TEST(Utf8Test, preserves_embedded_nulls)
	{
		EXPECT_EQ(Utf8ToString(std::u8string(u8"a\0b", 3)), std::string("a\0b", 3));
	}

	TEST(Utf8Test, handles_empty_strings)
	{
		EXPECT_TRUE(Utf8ToString(u8"").empty());
	}

	TEST(Utf8Test, round_trips_generic_filesystem_paths)
	{
		const std::filesystem::path path(u8"assets/\u00e9\u4e16\U0001f680.png");
		const auto text = Utf8ToString(path.generic_u8string());
		EXPECT_EQ(path, Utf8ToPath(text));
	}
}
