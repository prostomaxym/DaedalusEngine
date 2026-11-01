#include <gtest/gtest.h>
#include <Renderer/Resources/DecodedImage.h>
#include <Threads/JobSystem.h>

#include <array>
#include <utility>
#include <vector>

using namespace Daedalus;

namespace
{
	constexpr std::array<unsigned char, 17> ImageBytes{
		'P', '6', '\n', '1', ' ', '2', '\n', '2', '5', '5', '\n',
		255, 0, 0, 0, 0, 255
	};
}

TEST(DecodedImageTest, memory_decode_reports_dimensions_and_pixels)
{
	DecodedImage image;
	ASSERT_TRUE(image.LoadMemory(ImageBytes, false));
	EXPECT_EQ(image.GetWidth(), 1);
	EXPECT_EQ(image.GetHeight(), 2);
	EXPECT_EQ(image.GetChannels(), 3);
	EXPECT_EQ(image.GetPixels()[0], 255);
	EXPECT_EQ(image.GetPixels()[2], 0);
}

TEST(DecodedImageTest, invalid_decode_clears_previous_pixels_and_dimensions)
{
	DecodedImage image;
	ASSERT_TRUE(image.LoadMemory(ImageBytes));
	EXPECT_FALSE(image.LoadMemory({}));
	EXPECT_EQ(image.GetPixels(), nullptr);
	EXPECT_EQ(image.GetWidth(), 0);
	EXPECT_EQ(image.GetHeight(), 0);
	EXPECT_EQ(image.GetChannels(), 0);
	const std::array<unsigned char, 4> invalid{ 1, 2, 3, 4 };
	EXPECT_FALSE(image.LoadMemory(invalid));
	EXPECT_EQ(image.GetPixels(), nullptr);
}

TEST(DecodedImageTest, move_transfers_pixel_ownership)
{
	DecodedImage image;
	ASSERT_TRUE(image.LoadMemory(ImageBytes));
	auto* pixels = image.GetPixels();
	DecodedImage moved(std::move(image));
	EXPECT_EQ(moved.GetPixels(), pixels);
	EXPECT_EQ(image.GetPixels(), nullptr);
	DecodedImage assigned;
	assigned = std::move(moved);
	EXPECT_EQ(assigned.GetPixels(), pixels);
	EXPECT_EQ(moved.GetPixels(), nullptr);
}

TEST(DecodedImageTest, concurrent_decodes_use_independent_flip_settings)
{
	JobSystem jobs(4);
	std::array<DecodedImage, 32> images;
	std::vector<JobHandle> handles;
	for (size_t index = 0; index < images.size(); ++index)
		handles.push_back(jobs.Submit([&, index]
		{
			EXPECT_TRUE(images[index].LoadMemory(ImageBytes, index % 2 == 0));
		}));
	ASSERT_TRUE(jobs.Wait(jobs.WhenAll(handles)));
	for (size_t index = 0; index < images.size(); ++index)
	{
		ASSERT_NE(images[index].GetPixels(), nullptr);
		EXPECT_EQ(images[index].GetPixels()[0], index % 2 == 0 ? 0 : 255);
		EXPECT_EQ(images[index].GetPixels()[2], index % 2 == 0 ? 255 : 0);
	}
}

TEST(DecodedImageTest, decode_dependency_prepares_pixels_for_owner_thread)
{
	JobSystem jobs(1);
	DecodedImage image;
	const auto owner = std::this_thread::get_id();
	const std::array dependencies{ jobs.Submit([&] { EXPECT_TRUE(image.LoadMemory(ImageBytes)); }) };
	bool consumed = false;
	auto upload = jobs.Submit([&]
	{
		EXPECT_EQ(std::this_thread::get_id(), owner);
		ASSERT_NE(image.GetPixels(), nullptr);
		EXPECT_EQ(image.GetPixels()[2], 255);
		consumed = true;
	}, dependencies, JobAffinity::MainThread);
	ASSERT_TRUE(jobs.Wait(upload));
	EXPECT_TRUE(consumed);
}
