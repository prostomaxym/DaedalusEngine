#include <ECS/UUID.h>

#include <gtest/gtest.h>

#include <unordered_map>
#include <unordered_set>

using Daedalus::UUID;

TEST(UUIDTest, explicit_value_is_preserved)
{
	const Daedalus::UUID uuid(12345u);

	EXPECT_EQ(static_cast<uint64_t>(uuid), 12345u);
}

TEST(UUIDTest, generated_uuids_are_unique)
{
	std::unordered_set<uint64_t> values;
	for (int i = 0; i < 1000; ++i)
	{
		values.insert(static_cast<uint64_t>(Daedalus::UUID()));
	}

	EXPECT_EQ(values.size(), 1000u);
}

TEST(UUIDTest, can_be_used_as_hash_map_key)
{
	std::unordered_map<Daedalus::UUID, int> map;
	map[Daedalus::UUID(1u)] = 10;
	map[Daedalus::UUID(2u)] = 20;

	EXPECT_EQ(map.at(Daedalus::UUID(1u)), 10);
	EXPECT_EQ(map.at(Daedalus::UUID(2u)), 20);
	EXPECT_EQ(map.count(Daedalus::UUID(3u)), 0u);
}

TEST(UUIDTest, hash_matches_value)
{
	EXPECT_EQ(std::hash<Daedalus::UUID>()(Daedalus::UUID(42u)), 42u);
}
