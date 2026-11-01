#include <Debug/ConsoleCompletion.h>
#include <Config/CVarRegistry.h>
#include <gtest/gtest.h>
#include <limits>

using Daedalus::ConsoleCompletion;
using Daedalus::CVarRegistry;

namespace
{
	class ConsoleCompletionTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_TRUE(m_cvars.Register("graphics.gamma_value", "", [] { return 2.2f; }));
			ASSERT_TRUE(m_cvars.Register("graphics.fps_lock", "", [] { return 60; }));
			ASSERT_TRUE(m_cvars.Register("graphics.gamma", "", [] { return true; }));
		}

		std::string Hint(std::string_view input)
		{
			return ConsoleCompletion::Hint(input, input.size(), false, 256, m_cvars);
		}
		CVarRegistry m_cvars;
	};
}

TEST_F(ConsoleCompletionTest, CommandHintIsOnlyTheUntypedSuffix)
{
	EXPECT_EQ(Hint("ver"), "sion");
	EXPECT_EQ(Hint("qui"), "t");
	EXPECT_EQ(Hint("ge"), "t");
	EXPECT_EQ(Hint("he"), "lp");
	EXPECT_TRUE(Hint("version").empty());
	EXPECT_EQ(std::string("ver") + Hint("ver"), "version");
}

TEST_F(ConsoleCompletionTest, AmbiguousPrefixAcceptsTheFirstDisplayedMatch)
{
	const auto matches = ConsoleCompletion::Find("c", 1, m_cvars);
	EXPECT_EQ(matches.candidates, (std::vector<std::string>{ "clear", "cvars" }));
	EXPECT_EQ(Hint("c"), "lear");
	EXPECT_EQ(std::string("c") + Hint("c"), matches.candidates.front());
	EXPECT_EQ(Hint("cv"), "ars");
}

TEST_F(ConsoleCompletionTest, CVarHintsApplyToAllSupportedFirstArguments)
{
	for (const auto* verb : { "get", "set", "cvars" })
	{
		const auto input = std::string(verb) + " graphics.f";
		EXPECT_EQ(Hint(input), "ps_lock");
		EXPECT_EQ(input + Hint(input), std::string(verb) + " graphics.fps_lock");
	}
	EXPECT_EQ(Hint("get "), "graphics.fps_lock");
	EXPECT_EQ(Hint("get graphics.g"), "amma");
	EXPECT_TRUE(Hint("get graphics.gamma").empty());
}

TEST_F(ConsoleCompletionTest, BlankInputHasNoGhostHintButTabCanListCommands)
{
	EXPECT_TRUE(Hint("").empty());
	EXPECT_TRUE(Hint(" \t ").empty());
	const auto matches = ConsoleCompletion::Find("", 0, m_cvars);
	EXPECT_EQ(matches.candidates, (std::vector<std::string>{ "actions", "axes", "bind_action", "bind_axis", "clear", "cvars", "get", "help", "input_help", "quit", "set", "version" }));
}

TEST_F(ConsoleCompletionTest, ValuesUnknownCommandsAndUnknownPrefixesHaveNoHint)
{
	for (const auto* input : { "unknown", "get missing", "version graphics", "set graphics.fps_lock 6", "get graphics.fps_lock ", "getx graphics.f" })
		EXPECT_TRUE(Hint(input).empty()) << input;
}

TEST_F(ConsoleCompletionTest, SelectionAndMidLineEditingHideHints)
{
	EXPECT_TRUE(ConsoleCompletion::Hint("ver", 3, true, 256, m_cvars).empty());
	EXPECT_TRUE(ConsoleCompletion::Hint("version", 3, false, 256, m_cvars).empty());
	EXPECT_TRUE(ConsoleCompletion::Hint("set graphics.f 60", 14, false, 256, m_cvars).empty());
}

TEST_F(ConsoleCompletionTest, BufferFitIncludesTheNullTerminator)
{
	EXPECT_EQ(ConsoleCompletion::Hint("ver", 3, false, 8, m_cvars), "sion");
	EXPECT_TRUE(ConsoleCompletion::Hint("ver", 3, false, 7, m_cvars).empty());
	EXPECT_TRUE(ConsoleCompletion::Hint("ver", 3, false, 3, m_cvars).empty());
	EXPECT_TRUE(ConsoleCompletion::Hint("ver", 3, false, 0, m_cvars).empty());
	const std::string input = "get graphics.f";
	const size_t completed_size = std::string("get graphics.fps_lock").size();
	EXPECT_EQ(ConsoleCompletion::Hint(input, input.size(), false, completed_size + 1, m_cvars), "ps_lock");
	EXPECT_TRUE(ConsoleCompletion::Hint(input, input.size(), false, completed_size, m_cvars).empty());
}

TEST_F(ConsoleCompletionTest, MidLineMatchesPreserveFollowingValueText)
{
	const std::string input = "set graphics.f 60";
	const size_t cursor = std::string("set graphics.f").size();
	const auto matches = ConsoleCompletion::Find(input, cursor, m_cvars);
	ASSERT_EQ(matches.candidates.size(), 1);
	EXPECT_EQ(matches.offset, 4);
	EXPECT_EQ(matches.length, std::string("graphics.f").size());
	EXPECT_EQ(input.substr(0, matches.offset) + matches.candidates[0] + input.substr(matches.offset + matches.length),
		"set graphics.fps_lock 60");
}

TEST_F(ConsoleCompletionTest, LeadingAndArgumentWhitespaceArePreserved)
{
	EXPECT_EQ(Hint("  ver"), "sion");
	EXPECT_EQ(Hint("  get\t graphics.f"), "ps_lock");
	const std::string input = "  get\t graphics.f";
	EXPECT_EQ(input + Hint(input), "  get\t graphics.fps_lock");
}

TEST_F(ConsoleCompletionTest, InvalidCursorCannotReadOutsideTheInput)
{
	EXPECT_TRUE(ConsoleCompletion::Find("ver", 4, m_cvars).candidates.empty());
	EXPECT_TRUE(ConsoleCompletion::Find("ver", std::numeric_limits<size_t>::max(), m_cvars).candidates.empty());
	EXPECT_TRUE(ConsoleCompletion::Hint("ver", std::numeric_limits<size_t>::max(), false, 256, m_cvars).empty());
}
