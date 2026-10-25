#include <Events/EventDispatcher.h>
#include <Events/KeyEvent.h>
#include <Events/ApplicationEvent.h>

#include <gtest/gtest.h>

using namespace Daedalus;

TEST(EventDispatcherTest, calls_handler_for_matching_event_type)
{
	KeyPressedEvent event(65, 0);
	int received_key = 0;

	const bool dispatched = EventDispatcher::ProcessEvent<KeyPressedEvent>(event, [&](KeyPressedEvent& e)
	{
		received_key = e.GetKeyCode();
		return true;
	});

	EXPECT_TRUE(dispatched);
	EXPECT_EQ(received_key, 65);
	EXPECT_TRUE(event.IsHandled());
}

TEST(EventDispatcherTest, skips_handler_for_different_event_type)
{
	KeyPressedEvent event(65, 0);
	bool called = false;

	const bool dispatched = EventDispatcher::ProcessEvent<WindowCloseEvent>(event, [&](WindowCloseEvent&)
	{
		called = true;
		return true;
	});

	EXPECT_FALSE(dispatched);
	EXPECT_FALSE(called);
	EXPECT_FALSE(event.IsHandled());
}

TEST(EventDispatcherTest, handler_result_controls_handled_flag)
{
	WindowCloseEvent event;

	const bool dispatched = EventDispatcher::ProcessEvent<WindowCloseEvent>(event, [](WindowCloseEvent&)
	{
		return false;
	});

	EXPECT_TRUE(dispatched);
	EXPECT_FALSE(event.IsHandled());
}
