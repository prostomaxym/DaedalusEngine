#include <gtest/gtest.h>
#include <ECS/Components.h>
#include <ECS/Systems/TransformPreparation.h>
#include <Threads/JobSystem.h>

#include <array>
#include <vector>

using namespace Daedalus;

namespace
{
	void CheckTransforms(size_t count)
	{
		JobSystem jobs(4);
		std::vector<TransformComponent> inputs(count);
		std::vector<glm::mat4> outputs(count);
		for (size_t index = 0; index < count; ++index)
		{
			const float value = static_cast<float>(index);
			inputs[index].translation = { value, -value, value * 0.5f };
			inputs[index].rotation = { value, value * 0.25f, -value };
			inputs[index].scale = { 0.5f, 2.0f, -1.0f };
		}
		ASSERT_TRUE(PrepareTransforms(jobs, inputs, outputs));
		for (size_t index = 0; index < count; ++index)
		{
			const auto expected = inputs[index].GetTransform();
			for (int column = 0; column < 4; ++column)
				for (int row = 0; row < 4; ++row)
					EXPECT_NEAR(outputs[index][column][row], expected[column][row], 0.00001f);
		}
	}
}

TEST(TransformPreparationTest, serial_batch_matches_component_matrices)
{
	CheckTransforms(5);
}

TEST(TransformPreparationTest, parallel_batch_matches_component_matrices)
{
	CheckTransforms(257);
}

TEST(TransformPreparationTest, empty_batch_succeeds)
{
	JobSystem jobs(1);
	EXPECT_TRUE(PrepareTransforms(jobs, {}, {}));
}

TEST(TransformPreparationTest, stopped_scheduler_rejects_parallel_batch)
{
	JobSystem jobs(1);
	jobs.Stop();
	std::array<TransformComponent, 128> inputs;
	std::array<glm::mat4, 128> outputs;
	EXPECT_FALSE(PrepareTransforms(jobs, inputs, outputs));
}

TEST(TransformPreparationTest, mismatched_output_size_is_rejected)
{
	JobSystem jobs(1);
	std::array<TransformComponent, 2> inputs;
	std::array<glm::mat4, 1> outputs{ glm::mat4(1.0f) };
	EXPECT_FALSE(PrepareTransforms(jobs, inputs, outputs));
	EXPECT_FLOAT_EQ(outputs[0][0][0], 1.0f);
}
