#include <Debug/InputCommands.h>
#include <Debug/ConsoleCompletion.h>
#include <Config/KeybindConfig.h>
#include <Config/CVarRegistry.h>
#include <Utils/KeyCodes.h>
#include <gtest/gtest.h>
#include <filesystem>

using namespace Daedalus;

namespace
{
	std::vector<std::string> Execute(const std::string& command)
	{
		std::vector<std::string> output;
		EXPECT_TRUE(InputCommands::Execute(command, output));
		return output;
	}
}

TEST(InputCommandsTest, ActionCreationAndUnbindingAreLiveAndPersistent)
{
	Execute("bind_action cmd_jump 32 0");
	ASSERT_NE(KeybindConfig::FindActionBind("cmd_jump"), nullptr);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("cmd_jump"), DL_KEY_SPACE);
	EXPECT_EQ(KeybindConfig::FindActionBind("cmd_jump")->gamepad_button, 0);
	const auto path = std::filesystem::temp_directory_path() / "dl_input_commands.ini";
	KeybindConfig::Save(path);
	Execute("bind_action cmd_jump -1 -1");
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("cmd_jump"), -1);
	KeybindConfig::Load(path);
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("cmd_jump"), DL_KEY_SPACE);
	EXPECT_EQ(KeybindConfig::FindActionBind("cmd_jump")->gamepad_button, 0);
	std::filesystem::remove(path);
}

TEST(InputCommandsTest, MalformedAndInvalidActionsLeaveBindingsUnchanged)
{
	Execute("bind_action cmd_validation 65 0");
	const auto revision = KeybindConfig::GetRevision();
	for (const auto* command : { "bind_action", "bind_action cmd_validation 66", "bind_action cmd_validation 66 1 extra",
		"bind_action cmd_validation 999999999999999999999 0", "bind_action cmd_validation 65.5 0",
		"bind_action cmd_validation 100 0", "bind_action cmd_validation 66 15", "bind_action bad-name 66 0" })
	{
		EXPECT_FALSE(Execute(command).empty());
		EXPECT_EQ(KeybindConfig::GetRevision(), revision) << command;
	}
	EXPECT_EQ(KeybindConfig::GetKeyboardBind("cmd_validation"), DL_KEY_A);
}

TEST(InputCommandsTest, AxisCommandsValidateReferencesAndSupportAnalogOnlyMappings)
{
	Execute("bind_action cmd_left 65 -1");
	Execute("bind_action cmd_right 68 -1");
	Execute("bind_axis cmd_move cmd_left cmd_right 0 -1 0.2");
	ASSERT_NE(KeybindConfig::FindAxisBind("cmd_move"), nullptr);
	EXPECT_EQ(KeybindConfig::FindAxisBind("cmd_move")->negative_action, "cmd_left");
	EXPECT_EQ(KeybindConfig::FindAxisBind("cmd_move")->positive_action, "cmd_right");
	EXPECT_FLOAT_EQ(KeybindConfig::FindAxisBind("cmd_move")->scale, -1.f);
	Execute("bind_axis cmd_trigger - - 4 1 0.1");
	ASSERT_NE(KeybindConfig::FindAxisBind("cmd_trigger"), nullptr);
	EXPECT_TRUE(KeybindConfig::FindAxisBind("cmd_trigger")->negative_action.empty());
	EXPECT_EQ(KeybindConfig::FindAxisBind("cmd_trigger")->gamepad_axis, GamepadAxis::LeftTrigger);
	const auto revision = KeybindConfig::GetRevision();
	for (const auto* command : { "bind_axis", "bind_axis cmd_move cmd_left cmd_right 0 1",
		"bind_axis cmd_move cmd_left cmd_right 0 1 0.2 extra", "bind_axis cmd_move missing cmd_right 0 1 0.2",
		"bind_axis cmd_move - - 6 1 0.2", "bind_axis cmd_move - - 0 2 0.2", "bind_axis cmd_move - - 0 1 1",
		"bind_axis cmd_move - - 0 nan 0.2", "bind_axis cmd_move - - 0 1 inf" })
	{
		EXPECT_FALSE(Execute(command).empty());
		EXPECT_EQ(KeybindConfig::GetRevision(), revision) << command;
	}
}

TEST(InputCommandsTest, ListingsAreSortedFilteredAndShowLiveValues)
{
	Execute("bind_action cmd_list_Z 90 1");
	Execute("bind_action cmd_list_A 65 -1");
	EXPECT_EQ(Execute("actions cmd_list_"), (std::vector<std::string>{ "cmd_list_A keyboard=65 gamepad=-1", "cmd_list_Z keyboard=90 gamepad=1" }));
	Execute("bind_action cmd_list_A 66 2");
	EXPECT_EQ(Execute("actions cmd_list_A"), (std::vector<std::string>{ "cmd_list_A keyboard=66 gamepad=2" }));
	Execute("bind_axis cmd_list_axis - - 0 1 0.2");
	EXPECT_EQ(Execute("axes cmd_list_axis"), (std::vector<std::string>{ "cmd_list_axis negative=- positive=- gamepad=0 scale=1 deadzone=0.2" }));
	EXPECT_EQ(Execute("actions cmd_no_matches"), (std::vector<std::string>{ "No matching actions" }));
	EXPECT_EQ(Execute("axes cmd_no_matches"), (std::vector<std::string>{ "No matching axes" }));
	EXPECT_EQ(Execute("actions cmd_list_ extra"), (std::vector<std::string>{ "Usage: actions [prefix]" }));
	EXPECT_FALSE(Execute("actions").empty());
	EXPECT_FALSE(Execute("axes").empty());
}

TEST(InputCommandsTest, HelpExplainsCodesAndUnknownCommandsAreNotConsumed)
{
	EXPECT_GE(Execute("input_help").size(), 5);
	EXPECT_EQ(Execute("input_help extra"), (std::vector<std::string>{ "Usage: input_help" }));
	std::vector<std::string> output;
	for (const auto* command : { "", "get keybind.cmd_jump", "version", "bind_actionx", "unknown" })
		EXPECT_FALSE(InputCommands::Execute(command, output));
	EXPECT_TRUE(output.empty());
}

TEST(InputCommandsTest, CompletionUsesCurrentSortedBindingsAndPreservesFollowingArguments)
{
	Execute("bind_action cmd_complete_Z 90 0");
	Execute("bind_action cmd_complete_A 65 1");
	CVarRegistry cvars;
	const std::string input = "bind_action cmd_complete_ 66 0";
	const size_t cursor = std::string("bind_action cmd_complete_").size();
	const auto matches = ConsoleCompletion::Find(input, cursor, cvars);
	EXPECT_EQ(matches.candidates, (std::vector<std::string>{ "cmd_complete_A", "cmd_complete_Z" }));
	EXPECT_EQ(input.substr(0, matches.offset) + matches.candidates.front() + input.substr(matches.offset + matches.length),
		"bind_action cmd_complete_A 66 0");
	const std::string hint_input = "actions cmd_complete_";
	EXPECT_EQ(ConsoleCompletion::Hint(hint_input, hint_input.size(), false, 256, cvars), "A");
	Execute("bind_axis cmd_complete_axis - - 0 1 0.2");
	EXPECT_EQ(InputCommands::Complete("bind_axis", "cmd_complete_"), (std::vector<std::string>{ "cmd_complete_axis" }));
	EXPECT_TRUE(ConsoleCompletion::Find("bind_action cmd_complete_A 6", 27, cvars).candidates.empty());
	EXPECT_EQ(ConsoleCompletion::Hint("bind_a", 6, false, 256, cvars), "ction");
}
