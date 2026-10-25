#include <Events/EventsEngine.h>
#include <Events/Event.h>

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

	template<typename Pred>
	bool WaitFor(Pred pred, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000))
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (!pred())
		{
			if (std::chrono::steady_clock::now() > deadline)
				return false;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return true;
	}

	constexpr auto long_ttl = std::chrono::milliseconds(60000);
}

TEST(EventEngineTest, dispatches_queued_event)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));

	EXPECT_TRUE(WaitFor([&] { return count == 1; }));
}

TEST(EventEngineTest, event_with_zero_time_to_live_is_dropped)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, std::chrono::milliseconds(0)));
	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));

	EXPECT_TRUE(WaitFor([&] { return count >= 1; }));
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_EQ(count, 1);
}

TEST(EventEngineTest, paused_engine_holds_events_until_resumed)
{
	std::atomic<int> count{ 0 };
	EventEngine engine;
	engine.Start();
	engine.Pause();

	engine.AddEvent(std::make_unique<TestQueuedEvent>([&] { ++count; }, long_ttl));
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_EQ(count, 0);

	engine.Resume();
	EXPECT_TRUE(WaitFor([&] { return count == 1; }));
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

	EXPECT_TRUE(WaitFor([&] { return count == threads * per_thread; }, std::chrono::milliseconds(5000)));
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

	EXPECT_TRUE(WaitFor([&] { return count == 1; }));
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
