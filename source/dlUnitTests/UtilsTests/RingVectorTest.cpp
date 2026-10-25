#include <Utils/RingVector.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using Daedalus::RingVector;

TEST(RingVectorTest, new_buffer_is_empty)
{
	RingVector<int> buffer(3);

	EXPECT_TRUE(buffer.empty());
	EXPECT_EQ(buffer.size(), 0u);
	EXPECT_EQ(buffer.capacity(), 3u);
}

TEST(RingVectorTest, push_back_grows_until_capacity)
{
	RingVector<int> buffer(3);
	buffer.push_back(1);
	buffer.push_back(2);

	EXPECT_EQ(buffer.size(), 2u);
	EXPECT_EQ(buffer.back(), 2);
	EXPECT_EQ(buffer[0], 1);
	EXPECT_EQ(buffer[1], 2);
}

TEST(RingVectorTest, push_back_overwrites_oldest_when_full)
{
	RingVector<int> buffer(3);
	for (int i = 1; i <= 5; ++i)
	{
		buffer.push_back(i);
	}

	EXPECT_EQ(buffer.size(), 3u);
	EXPECT_EQ(buffer.linearize(), (std::vector<int>{ 3, 4, 5 }));
	EXPECT_EQ(buffer.back(), 5);
}

TEST(RingVectorTest, clear_resets_size)
{
	RingVector<int> buffer(3);
	buffer.push_back(1);
	buffer.push_back(2);
	buffer.clear();

	EXPECT_TRUE(buffer.empty());
	EXPECT_EQ(buffer.capacity(), 3u);

	buffer.push_back(7);
	EXPECT_EQ(buffer.linearize(), (std::vector<int>{ 7 }));
}

TEST(RingVectorTest, back_on_empty_buffer_throws)
{
	RingVector<int> buffer(3);

	EXPECT_THROW(buffer.back(), std::out_of_range);
}

TEST(RingVectorTest, fixed_capacity_template_parameter)
{
	RingVector<int, 4> buffer;

	EXPECT_EQ(buffer.capacity(), 4u);
	EXPECT_TRUE(buffer.empty());
}

TEST(RingVectorTest, iterates_in_insertion_order_after_wrap)
{
	RingVector<int> buffer(3);
	for (int i = 1; i <= 4; ++i)
	{
		buffer.push_back(i);
	}

	std::vector<int> forward(buffer.begin(), buffer.end());
	std::vector<int> backward(buffer.rbegin(), buffer.rend());

	EXPECT_EQ(forward, (std::vector<int>{ 2, 3, 4 }));
	EXPECT_EQ(backward, (std::vector<int>{ 4, 3, 2 }));
}

TEST(RingVectorTest, resize_fills_buffer_with_value)
{
	RingVector<int> buffer(2);
	buffer.resize(4, 9);

	EXPECT_EQ(buffer.capacity(), 4u);
	EXPECT_EQ(buffer.linearize(), (std::vector<int>{ 9, 9, 9, 9 }));
}
