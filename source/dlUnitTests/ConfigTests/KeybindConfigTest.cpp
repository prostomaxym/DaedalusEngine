#include <Macros.h>
#include <Config/KeybindConfig.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using Daedalus::KeybindConfig;

namespace
{
	std::filesystem::path TempConfigPath(const char* name)
	{
		return std::filesystem::temp_directory_path() / name;
	}
}

TEST(KeybindConfigTest, set_and_get_keyboard_bind)
{
	KeybindConfig::SetKeyboardBind("test_set_get", 65);

	EXPECT_EQ(KeybindConfig::GetKeyboardBind("test_set_get"), 65);
	EXPECT_FALSE(KeybindConfig::IsEmpty());
}

TEST(KeybindConfigTest, set_overwrites_existing_bind)
{
	KeybindConfig::SetKeyboardBind("test_overwrite", 1);
	KeybindConfig::SetKeyboardBind("test_overwrite", 2);

	EXPECT_EQ(KeybindConfig::GetKeyboardBind("test_overwrite"), 2);
}

TEST(KeybindConfigTest, save_then_load_restores_binds)
{
	const auto path = TempConfigPath("dl_keybind_roundtrip.ini");
	KeybindConfig::SetKeyboardBind("test_roundtrip", 32);
	KeybindConfig::Save(path);

	KeybindConfig::SetKeyboardBind("test_roundtrip", 1);
	KeybindConfig::Load(path);

	EXPECT_EQ(KeybindConfig::GetKeyboardBind("test_roundtrip"), 32);
	std::filesystem::remove(path);
}

TEST(KeybindConfigTest, load_missing_file_keeps_existing_binds)
{
	KeybindConfig::SetKeyboardBind("test_missing_file", 7);

	KeybindConfig::Load(TempConfigPath("dl_keybind_does_not_exist.ini"));

	EXPECT_EQ(KeybindConfig::GetKeyboardBind("test_missing_file"), 7);
}

TEST(KeybindConfigTest, load_reads_keyboard_section)
{
	const auto path = TempConfigPath("dl_keybind_handwritten.ini");
	{
		std::ofstream file(path);
		file << "[Keyboard]\ntest_handwritten = 87\n";
	}

	KeybindConfig::Load(path);

	EXPECT_EQ(KeybindConfig::GetKeyboardBind("test_handwritten"), 87);
	std::filesystem::remove(path);
}
