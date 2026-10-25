#include <Events/GamepadEvent.h>

#include <gtest/gtest.h>

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
