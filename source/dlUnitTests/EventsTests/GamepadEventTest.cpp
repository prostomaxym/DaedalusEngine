#include <Events/GamepadEvent.h>
#include <Platform/GLFW/GamepadButtonEvents.h>

#include <gtest/gtest.h>
#include <vector>

using namespace Daedalus;

TEST(GamepadEventTest, pressed_event_exposes_data_and_type)
{
	GpadPressedEvent event(3, 2);

	EXPECT_EQ(event.GetKeyCode(), 3);
	EXPECT_EQ(event.GetRepeatCount(), 2);
	EXPECT_EQ(event.GetEventType(), EventType::GamepadPressed);
	EXPECT_STREQ(event.GetNameC(), "GamepadPressed");
	EXPECT_EQ(event.GetInfo(), "GpadPressedEvent: 3 (2 repeats)");
}

TEST(GamepadEventTest, released_event_exposes_data_and_type)
{
	GpadReleasedEvent event(5);

	EXPECT_EQ(event.GetKeyCode(), 5);
	EXPECT_EQ(event.GetEventType(), EventType::GamepadReleased);
	EXPECT_EQ(event.GetInfo(), "GpadReleasedEvent: 5");
}

TEST(GamepadEventTest, button_events_are_gamepad_button_and_input)
{
	GpadPressedEvent event(0, 0);

	EXPECT_TRUE(event.IsInCategory(EventCategory::GamepadButtonEvent));
	EXPECT_TRUE(event.IsInCategory(EventCategory::InputEvent));
	EXPECT_FALSE(event.IsInCategory(EventCategory::GamepadAnalogEvent));
	EXPECT_FALSE(event.IsInCategory(EventCategory::KeyboardEvent));
}

TEST(GamepadEventTest, stick_event_keeps_fractional_percentage)
{
	GpadStickEvent event(1, 0.5f, GpadStickEvent::Right);

	EXPECT_EQ(event.GetKeyCode(), 1);
	EXPECT_FLOAT_EQ(event.GetPercentagePushed(), 0.5f);
	EXPECT_EQ(event.GetEventType(), EventType::GamepadStick);
	EXPECT_NE(event.GetInfo().find("-Right"), std::string::npos);
}

TEST(GamepadEventTest, stick_event_left_info)
{
	GpadStickEvent event(0, 1.f, GpadStickEvent::Left);

	EXPECT_NE(event.GetInfo().find("-Left"), std::string::npos);
}

TEST(GamepadEventTest, trigger_event_keeps_fractional_percentage)
{
	GpadTriggerEvent event(4, 0.25f, GpadTriggerEvent::Left);

	EXPECT_EQ(event.GetKeyCode(), 4);
	EXPECT_FLOAT_EQ(event.GetPercentagePushed(), 0.25f);
	EXPECT_EQ(event.GetEventType(), EventType::GamepadTrigger);
	EXPECT_NE(event.GetInfo().find("-Left"), std::string::npos);
}

TEST(GamepadEventTest, analog_events_are_analog_and_input)
{
	GpadTriggerEvent event(4, 0.f, GpadTriggerEvent::Right);

	EXPECT_TRUE(event.IsInCategory(EventCategory::GamepadAnalogEvent));
	EXPECT_TRUE(event.IsInCategory(EventCategory::InputEvent));
	EXPECT_FALSE(event.IsInCategory(EventCategory::GamepadButtonEvent));
}

TEST(GamepadEventTest, handled_flag_add_and_set)
{
	GpadReleasedEvent event(1);

	EXPECT_FALSE(event.IsHandled());
	event.AddHandle(true);
	event.AddHandle(false);
	EXPECT_TRUE(event.IsHandled());
	event.SetHandle(false);
	EXPECT_FALSE(event.IsHandled());
}

namespace
{
	class GamepadButtonEventsTest : public ::testing::Test
	{
	protected:
		struct RecordedEvent
		{
			EventType type;
			int button;
			int repeat;
		};

		void Poll(int milliseconds)
		{
			m_tracker.Update(m_buttons, GamepadButtonEvents::Clock::time_point{} + std::chrono::milliseconds(milliseconds),
				[this](Event& event)
				{
					EXPECT_TRUE(event.IsInCategory(EventCategory::GamepadButtonEvent));
					EXPECT_FALSE(event.IsInCategory(EventCategory::KeyboardEvent));
					const auto& button_event = static_cast<const GpadButtonEvent&>(event);
					const int repeat = event.GetEventType() == EventType::GamepadPressed
						? static_cast<const GpadPressedEvent&>(event).GetRepeatCount() : 0;
					m_events.push_back({ event.GetEventType(), button_event.GetKeyCode(), repeat });
				});
		}

		GamepadButtonEvents m_tracker;
		GamepadButtonEvents::Buttons m_buttons{};
		std::vector<RecordedEvent> m_events;
	};
}

TEST_F(GamepadButtonEventsTest, idle_buttons_emit_no_events)
{
	Poll(0);
	Poll(1000);
	EXPECT_TRUE(m_events.empty());
}

TEST_F(GamepadButtonEventsTest, press_and_release_emit_once_per_transition)
{
	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	Poll(0);
	Poll(100);
	ASSERT_EQ(m_events.size(), 1u);
	EXPECT_EQ(m_events[0].type, EventType::GamepadPressed);
	EXPECT_EQ(m_events[0].button, DL_GAMEPAD_BUTTON_A);
	EXPECT_EQ(m_events[0].repeat, 0);

	m_buttons[DL_GAMEPAD_BUTTON_A] = 0;
	Poll(200);
	Poll(1000);
	ASSERT_EQ(m_events.size(), 2u);
	EXPECT_EQ(m_events[1].type, EventType::GamepadReleased);
	EXPECT_EQ(m_events[1].button, DL_GAMEPAD_BUTTON_A);
}

TEST_F(GamepadButtonEventsTest, repeats_observe_initial_delay_and_interval)
{
	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	Poll(0);
	Poll(499);
	ASSERT_EQ(m_events.size(), 1u);
	Poll(500);
	ASSERT_EQ(m_events.size(), 2u);
	EXPECT_EQ(m_events[1].type, EventType::GamepadPressed);
	EXPECT_EQ(m_events[1].button, DL_GAMEPAD_BUTTON_A);
	EXPECT_EQ(m_events[1].repeat, 1);
	Poll(549);
	ASSERT_EQ(m_events.size(), 2u);
	Poll(550);
	ASSERT_EQ(m_events.size(), 3u);
	EXPECT_EQ(m_events[2].repeat, 1);
}

TEST_F(GamepadButtonEventsTest, slow_polls_do_not_burst_repeat_events)
{
	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	Poll(0);
	Poll(5000);
	Poll(5000);
	ASSERT_EQ(m_events.size(), 2u);
	EXPECT_EQ(m_events[1].repeat, 1);
	Poll(5050);
	ASSERT_EQ(m_events.size(), 3u);
}

TEST_F(GamepadButtonEventsTest, buttons_track_repeat_deadlines_independently)
{
	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	Poll(0);
	m_buttons[DL_GAMEPAD_BUTTON_LAST] = 1;
	Poll(100);
	Poll(500);
	ASSERT_EQ(m_events.size(), 3u);
	EXPECT_EQ(m_events[1].button, DL_GAMEPAD_BUTTON_LAST);
	EXPECT_EQ(m_events[1].repeat, 0);
	EXPECT_EQ(m_events[2].button, DL_GAMEPAD_BUTTON_A);
	EXPECT_EQ(m_events[2].repeat, 1);
	Poll(600);
	ASSERT_EQ(m_events.size(), 5u);
	EXPECT_EQ(m_events[4].button, DL_GAMEPAD_BUTTON_LAST);
	EXPECT_EQ(m_events[4].repeat, 1);
}

TEST_F(GamepadButtonEventsTest, disconnect_releases_buttons_and_reconnect_resets_repeats)
{
	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	m_buttons[DL_GAMEPAD_BUTTON_LAST] = 1;
	Poll(0);
	m_buttons.fill(0);
	Poll(500);
	Poll(1000);
	ASSERT_EQ(m_events.size(), 4u);
	EXPECT_EQ(m_events[2].type, EventType::GamepadReleased);
	EXPECT_EQ(m_events[2].button, DL_GAMEPAD_BUTTON_A);
	EXPECT_EQ(m_events[3].type, EventType::GamepadReleased);
	EXPECT_EQ(m_events[3].button, DL_GAMEPAD_BUTTON_LAST);

	m_buttons[DL_GAMEPAD_BUTTON_A] = 1;
	Poll(1100);
	Poll(1599);
	ASSERT_EQ(m_events.size(), 5u);
	EXPECT_EQ(m_events[4].repeat, 0);
	Poll(1600);
	ASSERT_EQ(m_events.size(), 6u);
	EXPECT_EQ(m_events[5].repeat, 1);
}
