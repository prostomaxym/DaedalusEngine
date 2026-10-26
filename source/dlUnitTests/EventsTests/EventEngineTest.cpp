#include <Events/EventsEngine.h>
#include <Events/Event.h>
#include <Events/ApplicationEvent.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

using namespace Daedalus;

namespace
{
	class TestQueuedEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(None)
		EVENT_CLASS_CATEGORY(NoneEvent)

		TestQueuedEvent(std::function<void()> func, std::chrono::milliseconds ttl)
			: m_func(std::move(func))
		{
			m_time_to_live = DeltaTime(std::chrono::duration_cast<std::chrono::nanoseconds>(ttl));
		}

		void Dispatch() override { m_func(); }

	private:
		std::function<void()> m_func;
	};

	constexpr auto long_ttl = std::chrono::milliseconds(60000);
}

TEST(EventEngineTest, dispatches_queued_event)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));

	EXPECT_EQ(count, 0);
	engine.DispatchPending();
	EXPECT_EQ(count, 1);
}

TEST(EventEngineTest, dispatches_events_in_fifo_order)
{
	EventEngine engine;
	engine.Start();
	engine.Pause();
	std::atomic<int> next{ 0 };
	std::atomic<bool> ordered{ true };
	for (int i = 0; i < 32; ++i)
		engine.AddEvent(std::make_unique<TestQueuedEvent>([&, i]
		{
			if (next.fetch_add(1) != i) ordered = false;
		}, long_ttl));
	engine.Resume();
	engine.DispatchPending();
	EXPECT_EQ(next, 32);
	engine.Stop();
	EXPECT_TRUE(ordered);
}

TEST(EventEngineTest, event_with_zero_time_to_live_is_dropped)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, std::chrono::milliseconds(0)));
	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));

	engine.DispatchPending();
	EXPECT_EQ(count, 1);
}

TEST(EventEngineTest, paused_engine_holds_events_until_resumed)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();
	engine.Pause();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));
	engine.DispatchPending();
	EXPECT_EQ(count, 0);

	engine.Resume();
	engine.DispatchPending();
	EXPECT_EQ(count, 1);
}

TEST(EventEngineTest, handles_events_queued_from_multiple_threads)
{
	constexpr int threads = 4;
	constexpr int per_thread = 200;
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	std::vector<std::thread> producers;
	for (int t = 0; t < threads; ++t)
	{
		producers.emplace_back([&]
		{
			for (int i = 0; i < per_thread; ++i)
				engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));
		});
	}
	for (auto& th : producers)
		th.join();

	EXPECT_EQ(count, 0);
	engine.DispatchPending();
	EXPECT_EQ(count, threads * per_thread);
}

TEST(EventEngineTest, handler_can_queue_another_event)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&]
	{
		engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));
	}, long_ttl));

	engine.DispatchPending();
	EXPECT_EQ(count, 0);
	engine.DispatchPending();
	EXPECT_EQ(count, 1);
}

TEST(EventEngineTest, stop_with_pending_events_does_not_hang)
{
	EventEngine engine;
	engine.Start();
	engine.Pause();
	for (int i = 0; i < 10; ++i)
		engine.AddEvent(std::make_unique<TestQueuedEvent>([] {}, long_ttl));

	engine.Stop();
	SUCCEED();
}

TEST(EventEngineTest, producer_thread_never_runs_handlers)
{
	EventEngine engine;
	engine.Start();
	const auto owner = std::this_thread::get_id();
	std::thread::id dispatched;
	std::thread producer([&]
	{
		engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { dispatched = std::this_thread::get_id(); }, long_ttl));
	});
	producer.join();
	EXPECT_EQ(dispatched, std::thread::id{});
	engine.DispatchPending();
	EXPECT_EQ(dispatched, owner);
}

TEST(EventEngineTest, stopped_queue_rejects_events_and_discards_pending)
{
	EventEngine engine;
	engine.Start();
	int count = 0;
	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));
	engine.Stop();
	EXPECT_FALSE(engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl)));
	engine.Start();
	engine.DispatchPending();
	EXPECT_EQ(count, 0);
}

TEST(EventEngineTest, cloned_window_event_retains_payload_after_callback_returns)
{
	EventEngine engine;
	engine.Start();
	{
		WindowResizeEvent event(640, 480);
		engine.AddEvent(event.Clone());
	}
	int count = 0;
	engine.DispatchPending([&](Event& event)
	{
		auto& resized = static_cast<WindowResizeEvent&>(event);
		EXPECT_EQ(resized.GetWidth(), 640u);
		EXPECT_EQ(resized.GetHeight(), 480u);
		++count;
	});
	EXPECT_EQ(count, 1);
}
