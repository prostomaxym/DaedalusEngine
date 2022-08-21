#include <DummyTest.h>

#include <gtest/gtest.h>

TEST(DummySumTest, plus) 
{
	EXPECT_EQ(DummyTest::sum(5, 5), 10);
	EXPECT_EQ(DummyTest::sum(1, 2), 3);
	EXPECT_EQ(DummyTest::sum(546, 651), 1197);
	EXPECT_EQ(DummyTest::sum(322, 0), 322);
}

TEST(DummySumTest, minus)
{
	EXPECT_EQ(DummyTest::sum(5, -5), 0);
	EXPECT_EQ(DummyTest::sum(-5, -5), -10);
	EXPECT_EQ(DummyTest::sum(0,-10), -10);
	EXPECT_EQ(DummyTest::sum(-322, 0), -322);
}

TEST(DummySumTest, zero)
{
	EXPECT_EQ(DummyTest::sum(0, 0), 0);
	EXPECT_EQ(DummyTest::sum(-0, 0), 0);
	EXPECT_EQ(DummyTest::sum(0, -0), 0);
	EXPECT_EQ(DummyTest::sum(-0, -0), 0);
}

TEST(DummySumTest, floating)
{
	EXPECT_EQ(DummyTest::sum(5.5, 5.5), 10);
	EXPECT_EQ(DummyTest::sum(5.9999, 5.9999), 10);
	EXPECT_EQ(DummyTest::sum(5.0000001, 5.0515123), 10);
	EXPECT_EQ(DummyTest::sum(0.000, 0.00000), 0);
}