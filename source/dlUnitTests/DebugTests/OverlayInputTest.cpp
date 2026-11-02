#include <Config/KeybindConfig.h>
#include <Debug/ConsoleOverlay.h>
#include <Debug/DebugOverlay.h>
#include <Events/GamepadEvent.h>
#include <Utils/KeyCodes.h>

#include <gtest/gtest.h>

using namespace Daedalus;

namespace
{
	class OverlayInputTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			if (const auto* binding = KeybindConfig::FindActionBind("Console"))
				m_console_binding = *binding;
			if (const auto* binding = KeybindConfig::FindActionBind("Debug"))
				m_debug_binding = *binding;
			ASSERT_TRUE(KeybindConfig::SetActionBind("Console", { DL_KEY_GRAVE_ACCENT, DL_GAMEPAD_BUTTON_A }));
			ASSERT_TRUE(KeybindConfig::SetActionBind("Debug", { DL_KEY_F1, DL_GAMEPAD_BUTTON_B }));
		}

		void TearDown() override
		{
			EXPECT_TRUE(KeybindConfig::SetActionBind("Console", m_console_binding));
			EXPECT_TRUE(KeybindConfig::SetActionBind("Debug", m_debug_binding));
		}

		void ExpectIgnored(Event& event)
		{
			for (const bool shown : { false, true })
			{
				m_console.Show(shown);
				m_debug.Show(shown);
				m_console.OnEvent(event);
				m_debug.OnEvent(event);
				EXPECT_FALSE(event.IsHandled());
				EXPECT_EQ(m_console.IsShown(), shown);
				EXPECT_EQ(m_debug.IsShown(), shown);
			}
		}

		InputActionBinding m_console_binding;
		InputActionBinding m_debug_binding;
		ConsoleOverlay m_console;
		DebugOverlay m_debug;
	};
}

TEST_F(OverlayInputTest, gamepad_press_and_repeat_do_not_toggle_overlays)
{
	for (const int button : { DL_GAMEPAD_BUTTON_A, DL_GAMEPAD_BUTTON_B })
	{
		for (const int repeat : { 0, 1 })
		{
			GpadPressedEvent event(button, repeat);
			ExpectIgnored(event);
		}
	}
}

TEST_F(OverlayInputTest, unrelated_gamepad_release_does_not_toggle_overlays)
{
	GpadReleasedEvent event(DL_GAMEPAD_BUTTON_X);
	ExpectIgnored(event);
}

TEST_F(OverlayInputTest, unbound_gamepad_buttons_do_not_toggle_overlays)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("Console", { DL_KEY_GRAVE_ACCENT, -1 }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("Debug", { DL_KEY_F1, -1 }));
	for (const int button : { -1, DL_GAMEPAD_BUTTON_A, DL_GAMEPAD_BUTTON_B })
	{
		GpadReleasedEvent event(button);
		ExpectIgnored(event);
	}
}

TEST_F(OverlayInputTest, rebinding_gamepad_buttons_stops_previous_bindings_toggling_overlays)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("Console", { DL_KEY_GRAVE_ACCENT, DL_GAMEPAD_BUTTON_X }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("Debug", { DL_KEY_F1, DL_GAMEPAD_BUTTON_Y }));
	for (const int button : { DL_GAMEPAD_BUTTON_A, DL_GAMEPAD_BUTTON_B })
	{
		GpadReleasedEvent event(button);
		ExpectIgnored(event);
	}
}
