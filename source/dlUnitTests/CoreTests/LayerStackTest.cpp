#include <gtest/gtest.h>

#include <Core/LayerStack.h>

#include <memory>
#include <string>
#include <vector>

using namespace Daedalus;

namespace
{
	class TestLayer : public Layer
	{
	public:
		TestLayer(int& attached, int& detached, bool main_layer = false, const std::string& name = "Test")
			: Layer(name, main_layer), m_attached(attached), m_detached(detached) {}

		void OnAttach() override { ++m_attached; }
		void OnDetach() override { ++m_detached; }

	private:
		int& m_attached;
		int& m_detached;
	};
}

TEST(LayerStackTest, push_layer_calls_on_attach)
{
	int attached = 0, detached = 0;
	LayerStack stack;
	stack.PushLayer(std::make_unique<TestLayer>(attached, detached));
	EXPECT_EQ(attached, 1);
	EXPECT_EQ(detached, 0);
}

TEST(LayerStackTest, layers_are_placed_before_overlays)
{
	int a = 0, d = 0;
	LayerStack stack;
	stack.PushOverlay(std::make_unique<TestLayer>(a, d, false, "overlay"));
	stack.PushLayer(std::make_unique<TestLayer>(a, d, false, "layer"));

	EXPECT_EQ((*stack.begin())->GetName(), "layer");
	EXPECT_EQ((*stack.rbegin())->GetName(), "overlay");
}

TEST(LayerStackTest, pop_layer_returns_layer_and_calls_on_detach)
{
	int a = 0, d = 0;
	LayerStack stack;
	auto layer = std::make_unique<TestLayer>(a, d);
	Layer* raw = layer.get();
	stack.PushLayer(std::move(layer));

	auto popped = stack.PopLayer(raw);
	EXPECT_EQ(popped.get(), raw);
	EXPECT_EQ(d, 1);
	EXPECT_TRUE(stack.begin() == stack.end());
}

TEST(LayerStackTest, pop_unknown_layer_returns_null)
{
	int a = 0, d = 0;
	LayerStack stack;
	TestLayer other(a, d);
	EXPECT_EQ(stack.PopLayer(&other), nullptr);
	EXPECT_EQ(d, 0);
}

TEST(LayerStackTest, pop_overlay_returns_overlay)
{
	int a = 0, d = 0;
	LayerStack stack;
	auto overlay = std::make_unique<TestLayer>(a, d);
	Layer* raw = overlay.get();
	stack.PushOverlay(std::move(overlay));

	auto popped = stack.PopOverlay(raw);
	EXPECT_EQ(popped.get(), raw);
	EXPECT_TRUE(stack.begin() == stack.end());
}

TEST(LayerStackTest, get_main_layer_finds_main_layer)
{
	int a = 0, d = 0;
	LayerStack stack;
	EXPECT_EQ(stack.GetMainLayer(), nullptr);

	stack.PushLayer(std::make_unique<TestLayer>(a, d, false, "other"));
	auto main_layer = std::make_unique<TestLayer>(a, d, true, "main");
	Layer* raw = main_layer.get();
	stack.PushLayer(std::move(main_layer));

	EXPECT_EQ(stack.GetMainLayer(), raw);
}

TEST(LayerStackTest, destructor_detaches_remaining_layers)
{
	int a = 0, d = 0;
	{
		LayerStack stack;
		stack.PushLayer(std::make_unique<TestLayer>(a, d));
		stack.PushOverlay(std::make_unique<TestLayer>(a, d));
	}
	EXPECT_EQ(d, 2);
}

TEST(LayerStackTest, clear_detaches_all_in_reverse_order_and_empties_stack)
{
	std::vector<std::string> order;
	struct OrderLayer : Layer
	{
		OrderLayer(std::vector<std::string>& o, const std::string& n) : Layer(n), m_order(o) {}
		void OnDetach() override { m_order.push_back(GetName()); }
		std::vector<std::string>& m_order;
	};

	LayerStack stack;
	stack.PushLayer(std::make_unique<OrderLayer>(order, "layer"));
	stack.PushOverlay(std::make_unique<OrderLayer>(order, "overlay"));
	stack.Clear();

	ASSERT_EQ(order.size(), 2u);
	EXPECT_EQ(order[0], "overlay");
	EXPECT_EQ(order[1], "layer");
	EXPECT_TRUE(stack.begin() == stack.end());
}

TEST(LayerStackTest, clear_does_not_detach_twice_on_destruction)
{
	int a = 0, d = 0;
	{
		LayerStack stack;
		stack.PushLayer(std::make_unique<TestLayer>(a, d));
		stack.Clear();
	}
	EXPECT_EQ(d, 1);
}

TEST(LayerStackTest, pop_overlay_finds_overlay_after_layers)
{
	int a = 0, d = 0;
	LayerStack stack;
	stack.PushLayer(std::make_unique<TestLayer>(a, d));
	auto overlay = std::make_unique<TestLayer>(a, d, false, "overlay");
	auto* raw = overlay.get();
	stack.PushOverlay(std::move(overlay));

	auto popped = stack.PopOverlay(raw);
	EXPECT_EQ(popped.get(), raw);
	EXPECT_EQ(d, 1);
}
