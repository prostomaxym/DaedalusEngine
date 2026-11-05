#include <Config/CVarRegistry.h>
#include <Config/GraphicsConfig.h>
#include <Config/KeybindConfig.h>
#include <Config/PathConfig.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <limits>

using Daedalus::CVarRegistry;
using Daedalus::GraphicsConfig;
using Daedalus::KeybindConfig;

TEST(CVarRegistryTest, RegistrationRejectsDuplicatesInvalidNamesAndMissingGetters)
{
	CVarRegistry registry;
	EXPECT_TRUE(registry.Register("test.value", "description", [] { return 7; }));
	EXPECT_FALSE(registry.Register("test.value", "duplicate", [] { return 8; }));
	EXPECT_FALSE(registry.Register("", "empty", [] { return 0; }));
	EXPECT_FALSE(registry.Register("bad name", "invalid", [] { return 0; }));
	EXPECT_FALSE(registry.Register("bad", "no getter", {}));
	std::string value, error;
	ASSERT_TRUE(registry.Get("test.value", value, error));
	EXPECT_EQ(value, "7");
}

TEST(CVarRegistryTest, GetReadsLiveValuesAndReportsUnknownNames)
{
	CVarRegistry registry;
	int current = 1;
	ASSERT_TRUE(registry.Register("value", "", [&] { return current; }));
	std::string value, error;
	ASSERT_TRUE(registry.Get("value", value, error));
	EXPECT_EQ(value, "1");
	current = 9;
	ASSERT_TRUE(registry.Get("value", value, error));
	EXPECT_EQ(value, "9");
	EXPECT_FALSE(registry.Get("missing", value, error));
	EXPECT_TRUE(value.empty());
	EXPECT_NE(error.find("Unknown CVar"), std::string::npos);
}

TEST(CVarRegistryTest, IntegerParsingAndValidationAreAtomic)
{
	CVarRegistry registry;
	int current = 60;
	ASSERT_TRUE(registry.Register("limit", "", [&] { return current; }, [&](const auto& value)
	{
		const auto number = std::get<int>(value);
		if (number < 0) return false;
		current = number;
		return true;
	}));
	std::string error;
	for (const auto* text : { "", " ", "12x", "1.5", "1 2", "2147483648", "-1", "999999999999999999999" })
	{
		EXPECT_FALSE(registry.Set("limit", text, error)) << text;
		EXPECT_EQ(current, 60);
	}
	EXPECT_TRUE(registry.Set("limit", " 144 ", error));
	EXPECT_EQ(current, 144);
	EXPECT_TRUE(error.empty());
}

TEST(CVarRegistryTest, BoolAcceptsOnlyExplicitBoolForms)
{
	CVarRegistry registry;
	bool current = false;
	ASSERT_TRUE(registry.Register("flag", "", [&] { return current; }, [&](const auto& value)
	{
		current = std::get<bool>(value);
		return true;
	}));
	std::string error;
	for (const auto* text : { "true", "1" })
	{
		ASSERT_TRUE(registry.Set("flag", text, error));
		EXPECT_TRUE(current);
	}
	for (const auto* text : { "false", "0" })
	{
		ASSERT_TRUE(registry.Set("flag", text, error));
		EXPECT_FALSE(current);
	}
	for (const auto* text : { "True", "yes", "2", "true false" })
		EXPECT_FALSE(registry.Set("flag", text, error));
}

TEST(CVarRegistryTest, FloatRejectsNonfiniteOverflowAndTrailingInput)
{
	CVarRegistry registry;
	float current = 0.5f;
	ASSERT_TRUE(registry.Register("radius", "", [&] { return current; }, [&](const auto& value)
	{
		current = std::get<float>(value);
		return true;
	}));
	std::string error;
	for (const auto* text : { "nan", "inf", "-inf", "1e100", "1.0junk", "" })
	{
		EXPECT_FALSE(registry.Set("radius", text, error)) << text;
		EXPECT_EQ(current, 0.5f);
	}
	ASSERT_TRUE(registry.Set("radius", "1.25e1", error));
	EXPECT_FLOAT_EQ(current, 12.5f);
}

TEST(CVarRegistryTest, StringValuesKeepInternalSpaces)
{
	CVarRegistry registry;
	std::string current;
	ASSERT_TRUE(registry.Register("text", "", [&] { return current; }, [&](const auto& value)
	{
		current = std::get<std::string>(value);
		return true;
	}));
	std::vector<std::string> output;
	EXPECT_TRUE(registry.Execute("set text two words", output));
	EXPECT_EQ(current, "two words");
	ASSERT_EQ(output.size(), 1);
	EXPECT_EQ(output[0], "text = two words");
}

TEST(CVarRegistryTest, ReadOnlyAndUnknownSetsDoNotMutate)
{
	CVarRegistry registry;
	ASSERT_TRUE(registry.Register("startup", "", [] { return 42; }));
	std::string error;
	EXPECT_FALSE(registry.Set("startup", "5", error));
	EXPECT_NE(error.find("Read-only"), std::string::npos);
	EXPECT_FALSE(registry.Set("missing", "5", error));
	EXPECT_NE(error.find("Unknown"), std::string::npos);
}

TEST(CVarRegistryTest, CommandSyntaxRejectsMissingOrExtraArguments)
{
	CVarRegistry registry;
	ASSERT_TRUE(registry.Register("value", "", [] { return 1; }));
	for (const auto* text : { "get", "set", "set value", "get value extra", "cvars value extra" })
	{
		std::vector<std::string> output;
		ASSERT_TRUE(registry.Execute(text, output));
		ASSERT_EQ(output.size(), 1);
		EXPECT_NE(output[0].find("Usage:"), std::string::npos) << text;
	}
	std::vector<std::string> output;
	EXPECT_FALSE(registry.Execute("version", output));
	EXPECT_TRUE(output.empty());
	EXPECT_TRUE(registry.Execute("  get value  ", output));
	EXPECT_EQ(output.back(), "value = 1");
}

TEST(CVarRegistryTest, ListingAndCompletionAreSortedAndPrefixFiltered)
{
	CVarRegistry registry;
	ASSERT_TRUE(registry.Register("graphics.z", "Z setting", [] { return true; }));
	ASSERT_TRUE(registry.Register("graphics.a", "A setting", [] { return 1.f; }));
	ASSERT_TRUE(registry.Register("paths.assets", "path", [] { return std::string("folder"); }));
	EXPECT_EQ(registry.Complete("graphics."), (std::vector<std::string>{ "graphics.a", "graphics.z" }));
	EXPECT_TRUE(registry.Complete("missing").empty());
	std::vector<std::string> output;
	ASSERT_TRUE(registry.Execute("cvars graphics.", output));
	ASSERT_EQ(output.size(), 2);
	EXPECT_EQ(output[0], "graphics.a = 1 [read-only] A setting");
	EXPECT_EQ(output[1], "graphics.z = true [read-only] Z setting");
	ASSERT_TRUE(registry.Execute("cvars missing", output));
	EXPECT_EQ(output.back(), "No matching CVars");
}

TEST(CVarRegistryTest, RegistriesDoNotShareBindings)
{
	CVarRegistry first, second;
	ASSERT_TRUE(first.Register("value", "", [] { return 1; }));
	EXPECT_TRUE(second.Complete("").empty());
}

class ConfigCVarTest : public ::testing::Test
{
protected:
	void SetUp() override { m_original_fps = GraphicsConfig::GetFPSLock(); }
	void TearDown() override { EXPECT_TRUE(GraphicsConfig::SetFPSLock(m_original_fps)); }
	int m_original_fps{ 0 };
};

TEST_F(ConfigCVarTest, FPSBindingChangesConfigAndRejectsNegativeValues)
{
	CVarRegistry registry;
	Daedalus::RegisterConfigCVars(registry);
	std::string error;
	ASSERT_TRUE(registry.Set("graphics.fps_lock", "120", error));
	EXPECT_EQ(GraphicsConfig::GetFPSLock(), 120);
	EXPECT_FALSE(registry.Set("graphics.fps_lock", "-1", error));
	EXPECT_EQ(GraphicsConfig::GetFPSLock(), 120);
	EXPECT_TRUE(registry.Set("graphics.fps_lock", "0", error));
	EXPECT_EQ(GraphicsConfig::GetFPSLock(), 0);
}

TEST_F(ConfigCVarTest, StartupBindingsAreReadOnly)
{
	CVarRegistry registry;
	Daedalus::RegisterConfigCVars(registry);
	std::string error, value;
	for (const auto* name : { "graphics.vsync", "graphics.gamma_value", "graphics.ssao", "graphics.window_width", "paths.assets", "paths.scripts", "paths.scene" })
	{
		ASSERT_TRUE(registry.Get(name, value, error));
		EXPECT_FALSE(registry.Set(name, "0", error));
		EXPECT_NE(error.find("Read-only"), std::string::npos);
	}
	ASSERT_TRUE(registry.Get("paths.scripts", value, error));
	EXPECT_EQ(value, Daedalus::Utf8ToString(Daedalus::PathConfig::GetScriptsPath().u8string()));
}

TEST_F(ConfigCVarTest, KeyboardBindingChangesExistingConfigAndValidatesKeyCodes)
{
	KeybindConfig::SetKeyboardBind("CVarTestKey", 65);
	CVarRegistry registry;
	Daedalus::RegisterConfigCVars(registry);
	std::string error;
	ASSERT_TRUE(registry.Set("keybind.CVarTestKey", "66", error));
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("CVarTestKey"), 66);
	for (const auto* code : { "-1", "0", "58", "270", "349", "99999999" })
		EXPECT_FALSE(registry.Set("keybind.CVarTestKey", code, error));
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("CVarTestKey"), 66);
	KeybindConfig::SetKeyboardBind("CVarTestKey", 65);
}

TEST_F(ConfigCVarTest, RuntimeFPSValuePersistsThroughExistingSaveLoad)
{
	const auto path = std::filesystem::temp_directory_path() / "dl_cvar_graphics_roundtrip.ini";
	ASSERT_TRUE(GraphicsConfig::SetFPSLock(144));
	GraphicsConfig::Save(path);
	ASSERT_TRUE(GraphicsConfig::SetFPSLock(0));
	GraphicsConfig::Load(path);
	EXPECT_EQ(GraphicsConfig::GetFPSLock(), 144);
	std::filesystem::remove(path);
}

TEST_F(ConfigCVarTest, FPSSetterRejectsInvalidInputWithoutMutation)
{
	ASSERT_TRUE(GraphicsConfig::SetFPSLock(60));
	EXPECT_FALSE(GraphicsConfig::SetFPSLock(-1));
	EXPECT_EQ(GraphicsConfig::GetFPSLock(), 60);
}
