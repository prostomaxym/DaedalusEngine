#include <Debug/ConsoleCommands.h>
#include <Debug/ConsoleCompletion.h>
#include <Config/CVarRegistry.h>
#include <gtest/gtest.h>
#include <sstream>

using Daedalus::ConsoleCommands;
using Daedalus::ConsoleCompletion;
using Daedalus::CVarRegistry;

TEST(ConsoleCommandsTest, HelpListsEveryCommandWithSyntaxAndDescription)
{
	std::vector<std::string> output;
	ASSERT_TRUE(ConsoleCommands::ExecuteHelp("help", output));
	const std::vector<std::string> syntaxes = {
		"actions [prefix]", "axes [prefix]",
		"bind_action <name> <keyboard-code|-1> <gamepad-button|-1>",
		"bind_axis <name> <negative-action|-> <positive-action|-> <gamepad-axis|-1> <scale> <deadzone>",
		"clear", "cvars [prefix]", "get <name>", "help", "input_help", "quit", "set <name> <value>", "version"
	};
	ASSERT_EQ(output.size(), syntaxes.size() + 1);
	EXPECT_EQ(output.front(), "Supported commands ([] optional, <> required):");
	for (size_t index = 0; index < syntaxes.size(); ++index)
	{
		const auto prefix = syntaxes[index] + " - ";
		EXPECT_EQ(output[index + 1].substr(0, prefix.size()), prefix);
		EXPECT_GT(output[index + 1].size(), prefix.size());
	}
}

TEST(ConsoleCommandsTest, HelpAndCompletionUseTheSameCommandCatalog)
{
	std::vector<std::string> output;
	ASSERT_TRUE(ConsoleCommands::ExecuteHelp("help", output));
	std::vector<std::string> names;
	for (size_t index = 1; index < output.size(); ++index)
	{
		std::istringstream line(output[index]);
		std::string name;
		line >> name;
		names.push_back(name);
	}
	EXPECT_EQ(names, ConsoleCommands::Complete(""));
	CVarRegistry cvars;
	EXPECT_EQ(ConsoleCompletion::Find("", 0, cvars).candidates, names);
	EXPECT_EQ(ConsoleCommands::Complete("he"), (std::vector<std::string>{ "help" }));
	EXPECT_EQ(ConsoleCompletion::Hint("he", 2, false, 256, cvars), "lp");
	EXPECT_TRUE(ConsoleCompletion::Hint("help", 4, false, 256, cvars).empty());
	EXPECT_TRUE(ConsoleCompletion::Find("help extra", 10, cvars).candidates.empty());
}

TEST(ConsoleCommandsTest, HelpAllowsSurroundingWhitespace)
{
	std::vector<std::string> expected;
	ASSERT_TRUE(ConsoleCommands::ExecuteHelp("help", expected));
	for (const auto* command : { " help ", "\thelp\t", "\r\nhelp \t\r\n" })
	{
		std::vector<std::string> output;
		EXPECT_TRUE(ConsoleCommands::ExecuteHelp(command, output));
		EXPECT_EQ(output, expected);
	}
}

TEST(ConsoleCommandsTest, HelpRejectsExtraArguments)
{
	for (const auto* command : { "help extra", "help version", "help extra more", "help\targument" })
	{
		std::vector<std::string> output;
		EXPECT_TRUE(ConsoleCommands::ExecuteHelp(command, output));
		EXPECT_EQ(output, (std::vector<std::string>{ "Usage: help" }));
	}
}

TEST(ConsoleCommandsTest, NonHelpCommandsAreNotConsumedAndOutputIsPreserved)
{
	std::vector<std::string> output{ "existing output" };
	for (const auto* command : { "", " \t", "HELP", "helpful", "version", "input_help", "get graphics.fps_lock" })
	{
		EXPECT_FALSE(ConsoleCommands::ExecuteHelp(command, output));
		EXPECT_EQ(output, (std::vector<std::string>{ "existing output" }));
	}
	ASSERT_TRUE(ConsoleCommands::ExecuteHelp("help", output));
	EXPECT_EQ(output.front(), "existing output");
	EXPECT_EQ(output.size(), 14);
}
