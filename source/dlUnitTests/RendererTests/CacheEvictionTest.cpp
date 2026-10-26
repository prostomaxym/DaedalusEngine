#include <Renderer/Resources/CacheEviction.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>

using namespace Daedalus;

TEST(CacheEvictionTest, keeps_resources_with_external_owners)
{
	std::map<int, std::shared_ptr<int>> cache;
	auto retained = std::make_shared<int>(1);
	cache[1] = retained;
	cache[2] = std::make_shared<int>(2);
	int released = 0;
	EXPECT_EQ(EvictUnused(cache, [](const auto& entry) -> const auto& { return entry.second; },
		[&](const auto& resource) { released = *resource; }), 1u);
	EXPECT_EQ(cache.size(), 1u);
	EXPECT_EQ(released, 2);
	EXPECT_EQ(cache.at(1), retained);
}

TEST(CacheEvictionTest, purges_set_entries_after_last_external_owner_releases)
{
	std::set<std::shared_ptr<int>> cache;
	auto resource = std::make_shared<int>(1);
	cache.insert(resource);
	const auto project = [](const auto& entry) -> const auto& { return entry; };
	const auto release = [](const auto&) {};
	EXPECT_EQ(EvictUnused(cache, project, release), 0u);
	resource.reset();
	EXPECT_EQ(EvictUnused(cache, project, release), 1u);
	EXPECT_TRUE(cache.empty());
}

TEST(CacheEvictionTest, null_entries_do_not_call_release_callback)
{
	std::map<int, std::shared_ptr<int>> cache;
	cache[1] = nullptr;
	bool called = false;
	EXPECT_EQ(EvictUnused(cache, [](const auto& entry) -> const auto& { return entry.second; },
		[&](const auto&) { called = true; }), 1u);
	EXPECT_FALSE(called);
}
