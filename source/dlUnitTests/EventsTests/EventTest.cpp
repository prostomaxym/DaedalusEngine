#include <Events/ApplicationEvent.h>
#include <Events/KeyEvent.h>
#include <Events/MouseEvent.h>

#include <gtest/gtest.h>

using namespace Daedalus;

TEST(EventTest, key_pressed_event_exposes_data_and_type)
{
	KeyPressedEvent event(65, 2);

	EXPECT_EQ(event.GetKeyCode(), 65);
	EXPECT_EQ(event.GetRepeatCount(), 2);
	EXPECT_EQ(event.GetEventType(), EventType::KeyPressed);
	EXPECT_EQ(KeyPressedEvent::GetStaticType(), EventType::KeyPressed);
	EXPECT_STREQ(event.GetNameC(), "KeyPressed");
}

TEST(EventTest, key_events_are_keyboard_and_input_but_not_mouse)
{
	KeyReleasedEvent event(65);

	EXPECT_TRUE(event.IsInCategory(EventCategory::KeyboardEvent));
	EXPECT_TRUE(event.IsInCategory(EventCategory::InputEvent));
	EXPECT_FALSE(event.IsInCategory(EventCategory::MouseEvent));
	EXPECT_FALSE(event.IsInCategory(EventCategory::ApplicationEvent));
}

TEST(EventTest, mouse_moved_event_exposes_position)
{
	MouseMovedEvent event(10.5f, -3.f);

	EXPECT_FLOAT_EQ(event.GetX(), 10.5f);
	EXPECT_FLOAT_EQ(event.GetY(), -3.f);
	EXPECT_EQ(event.GetEventType(), EventType::MouseMoved);
	EXPECT_TRUE(event.IsInCategory(EventCategory::MouseEvent));
	EXPECT_TRUE(event.IsInCategory(EventCategory::InputEvent));
}

TEST(EventTest, window_resize_event_exposes_size)
{
	WindowResizeEvent event(1280, 720);

	EXPECT_EQ(event.GetWidth(), 1280);
	EXPECT_EQ(event.GetHeight(), 720);
	EXPECT_EQ(event.GetEventType(), EventType::WindowResize);
	EXPECT_TRUE(event.IsInCategory(EventCategory::ApplicationEvent));
}

TEST(EventTest, event_info_contains_payload)
{
	KeyPressedEvent event(65, 2);

	const std::string info = event.GetInfo();
	EXPECT_NE(info.find("65"), std::string::npos);
	EXPECT_NE(info.find("2"), std::string::npos);
}

TEST(EventTest, handled_flag_set_and_add)
{
	WindowCloseEvent event;
	EXPECT_FALSE(event.IsHandled());

	event.AddHandle(false);
	EXPECT_FALSE(event.IsHandled());

	event.AddHandle(true);
	EXPECT_TRUE(event.IsHandled());

	event.AddHandle(false);
	EXPECT_TRUE(event.IsHandled()) << "AddHandle must not clear an already handled event";

	event.SetHandle(false);
	EXPECT_FALSE(event.IsHandled());
}
