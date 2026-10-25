#include <Debug/Log.h>

#include <gtest/gtest.h>

#include <cstring>

using Daedalus::Log;

TEST(LogTest, category_to_string)
{
	EXPECT_STREQ(Log::ToString(Log::Categories::EngineCore), "EngineCore");
	EXPECT_STREQ(Log::ToString(Log::Categories::Physics), "Physics");
	EXPECT_STREQ(Log::ToString(Log::Categories::RendererAPI), "RendererAPI");
}

TEST(LogTest, level_to_string)
{
	EXPECT_STREQ(Log::ToString(Log::Levels::Trace), "Trace");
	EXPECT_STREQ(Log::ToString(Log::Levels::Critical), "Critical");
}

TEST(LogTest, enable_and_disable_category)
{
	Log::EnableAllCategories();
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::ECS));

	Log::DisableCategory(Log::Categories::ECS);
	EXPECT_FALSE(Log::IsEnabled(Log::Categories::ECS));
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::Renderer));

	Log::EnableCategory(Log::Categories::ECS);
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::ECS));
}

TEST(LogTest, disable_category_twice_keeps_it_disabled)
{
	Log::EnableAllCategories();
	Log::DisableCategory(Log::Categories::Events);
	Log::DisableCategory(Log::Categories::Events);
	EXPECT_FALSE(Log::IsEnabled(Log::Categories::Events));
}

TEST(LogTest, toggle_category_only_affects_requested_bit)
{
	Log::EnableAllCategories();
	Log::ToggleCategory(Log::Categories::Platform, false);
	EXPECT_FALSE(Log::IsEnabled(Log::Categories::Platform));
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::Physics));
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::EngineCore));

	Log::ToggleCategory(Log::Categories::Platform, true);
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::Platform));
}

TEST(LogTest, enable_all_includes_physics)
{
	Log::EnableAllCategories();
	EXPECT_TRUE(Log::IsEnabled(Log::Categories::Physics));
}
