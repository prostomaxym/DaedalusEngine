#include <Config/KeybindConfig.h>
#include <Utils/KeyCodes.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace Daedalus;

TEST(InputBindingsTest, LegacyKeyboardRebindingPreservesGamepadAndRevision)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_jump", { DL_KEY_SPACE, DL_GAMEPAD_BUTTON_A }));
	const auto revision = KeybindConfig::GetRevision();
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_jump", { DL_KEY_SPACE, DL_GAMEPAD_BUTTON_A }));
	EXPECT_EQ(KeybindConfig::GetRevision(), revision);
	KeybindConfig::SetKeyboardBind("bindings_jump", DL_KEY_J);
	EXPECT_GT(KeybindConfig::GetRevision(), revision);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_jump")->gamepad_button, DL_GAMEPAD_BUTTON_A);
	EXPECT_EQ(KeybindConfig::GetKeyboardBinds().at("bindings_jump"), DL_KEY_J);
}

TEST(InputBindingsTest, InvalidActionsDoNotMutateConfiguration)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_valid", { DL_KEY_A, 0 }));
	const auto revision = KeybindConfig::GetRevision();
	for (const int key : { -2, 0, 100, 349, std::numeric_limits<int>::max() })
		EXPECT_FALSE(KeybindConfig::SetActionBind("bindings_valid", { key, 0 }));
	for (const int button : { -2, 15, std::numeric_limits<int>::max() })
		EXPECT_FALSE(KeybindConfig::SetActionBind("bindings_valid", { DL_KEY_B, button }));
	for (const auto* name : { "", "bad name", "bad=entry", "bad-section", "bad\nname" })
		EXPECT_FALSE(KeybindConfig::SetActionBind(name, { DL_KEY_A, 0 }));
	EXPECT_FALSE(KeybindConfig::SetActionBind(std::string(65, 'a'), {}));
	EXPECT_EQ(KeybindConfig::GetRevision(), revision);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_valid"), DL_KEY_A);
	KeybindConfig::SetKeyboardBind("bindings_valid", 100);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_valid"), DL_KEY_A);
}

TEST(InputBindingsTest, UnboundAndMissingBindingsAreExplicit)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_unbound", {}));
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_unbound"), -1);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_unbound")->gamepad_button, -1);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_does_not_exist"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_does_not_exist"), nullptr);
}

TEST(InputBindingsTest, AxisValidationRejectsInvalidReferencesAndNumbers)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_negative", { DL_KEY_A, -1 }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_positive", { DL_KEY_D, -1 }));
	InputAxisBinding binding{ "bindings_negative", "bindings_positive", GamepadAxis::LeftX, -1.f, 0.2f };
	ASSERT_TRUE(KeybindConfig::SetAxisBind("bindings_axis", binding));
	const auto revision = KeybindConfig::GetRevision();
	ASSERT_TRUE(KeybindConfig::SetAxisBind("bindings_axis", binding));
	EXPECT_EQ(KeybindConfig::GetRevision(), revision);
	for (const float value : { -0.1f, 1.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
	{
		auto invalid = binding;
		invalid.deadzone = value;
		EXPECT_FALSE(KeybindConfig::SetAxisBind("bindings_axis", invalid));
	}
	for (const float value : { -2.f, 2.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
	{
		auto invalid = binding;
		invalid.scale = value;
		EXPECT_FALSE(KeybindConfig::SetAxisBind("bindings_axis", invalid));
	}
	binding.negative_action = "missing_action";
	EXPECT_FALSE(KeybindConfig::SetAxisBind("bindings_axis", binding));
	binding.negative_action.clear();
	binding.gamepad_axis = static_cast<GamepadAxis>(6);
	EXPECT_FALSE(KeybindConfig::SetAxisBind("bindings_axis", binding));
	EXPECT_EQ(KeybindConfig::GetRevision(), revision);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_axis")->scale, -1.f);
}

TEST(InputBindingsTest, ActionAndAxisPersistenceRoundTripsCaseAndPrecision)
{
	const auto path = std::filesystem::temp_directory_path() / "dl_input_bindings_roundtrip.ini";
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_Case", { DL_KEY_A, DL_GAMEPAD_BUTTON_A }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_case", { DL_KEY_B, DL_GAMEPAD_BUTTON_B }));
	const InputAxisBinding expected{ "bindings_Case", "bindings_case", GamepadAxis::RightY, -0.87654321f, 0.12345678f };
	ASSERT_TRUE(KeybindConfig::SetAxisBind("bindings_persist_axis", expected));
	KeybindConfig::Save(path);
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_Case", {}));
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_case", {}));
	ASSERT_TRUE(KeybindConfig::SetAxisBind("bindings_persist_axis", {}));
	KeybindConfig::Load(path);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_Case"), DL_KEY_A);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_case"), DL_KEY_B);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_Case")->gamepad_button, DL_GAMEPAD_BUTTON_A);
	const auto& actual = *KeybindConfig::FindAxisBind("bindings_persist_axis");
	EXPECT_EQ(actual.negative_action, expected.negative_action);
	EXPECT_EQ(actual.positive_action, expected.positive_action);
	EXPECT_EQ(actual.gamepad_axis, expected.gamepad_axis);
	EXPECT_EQ(actual.scale, expected.scale);
	EXPECT_EQ(actual.deadzone, expected.deadzone);
	std::filesystem::remove(path);
}

TEST(InputBindingsTest, MalformedIniEntriesAreRejectedWithoutThrowingOrMutation)
{
	const auto path = std::filesystem::temp_directory_path() / "dl_input_bindings_invalid.ini";
	ASSERT_TRUE(KeybindConfig::SetActionBind("bindings_ini_valid", { DL_KEY_A, 0 }));
	ASSERT_TRUE(KeybindConfig::SetAxisBind("bindings_ini_axis", { {}, {}, GamepadAxis::LeftX, 1.f, 0.2f }));
	{
		std::ofstream file(path);
		file << "[Keyboard]\nbindings_ini_valid=999999999999999999999\nbindings_ini_bad=65junk\n"
			<< "[Gamepad]\nbindings_ini_valid=15\nbindings_ini_pad=2\n"
			<< "[Axes]\nbindings_ini_axis=- - 0 1 1\nbindings_ini_bad_axis=- missing 0 1 0.2\n"
			<< "bindings_ini_nan=- - 0 nan 0.2\nbindings_ini_extra=- - 0 1 0.2 extra\n"
			<< "bindings_ini_good_axis=- - 4 -1 0.125\n";
	}
	EXPECT_NO_THROW(KeybindConfig::Load(path));
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("bindings_ini_valid"), DL_KEY_A);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_ini_valid")->gamepad_button, 0);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_ini_axis")->deadzone, 0.2f);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_ini_bad"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_ini_bad_axis"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_ini_nan"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_ini_extra"), nullptr);
	ASSERT_NE(KeybindConfig::FindActionBind("bindings_ini_pad"), nullptr);
	EXPECT_EQ(KeybindConfig::FindActionBind("bindings_ini_pad")->keyboard, -1);
	ASSERT_NE(KeybindConfig::FindAxisBind("bindings_ini_good_axis"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("bindings_ini_good_axis")->gamepad_axis, GamepadAxis::LeftTrigger);
	std::filesystem::remove(path);
}
