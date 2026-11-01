#include <Renderer/Debug/DebugDrawBatch.h>
#include <Renderer/API/Renderer.h>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

using Daedalus::DebugDrawBatch;

TEST(DebugDrawBatchTest, LinePreservesPositionsColorsAndDepthGroups)
{
	DebugDrawBatch batch;
	ASSERT_TRUE(batch.Line({ 1.f, 2.f, 3.f }, { 4.f, 5.f, 6.f }, { 1.f, 0.f, 0.5f }));
	ASSERT_TRUE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f), false));
	EXPECT_EQ(batch.GetLineCount(), 2);
	ASSERT_EQ(batch.GetVertices(true).size(), 2);
	EXPECT_EQ(batch.GetVertices(true)[0].position, glm::vec3(1.f, 2.f, 3.f));
	EXPECT_EQ(batch.GetVertices(true)[1].position, glm::vec3(4.f, 5.f, 6.f));
	EXPECT_EQ(batch.GetVertices(true)[0].color, glm::vec3(1.f, 0.f, 0.5f));
	EXPECT_EQ(batch.GetVertices(false).size(), 2);
}

TEST(DebugDrawBatchTest, CapacityIsSharedBetweenDepthGroups)
{
	DebugDrawBatch batch(2);
	ASSERT_TRUE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	ASSERT_TRUE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f), false));
	EXPECT_FALSE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	EXPECT_EQ(batch.GetLineCount(), 2);
	EXPECT_EQ(batch.GetRejectedPrimitives(), 1);
}

TEST(DebugDrawBatchTest, BoxHasTwelveUniqueAxisAlignedEdges)
{
	DebugDrawBatch batch;
	const glm::vec3 minimum(-1.f, -2.f, -3.f), maximum(1.f, 2.f, 3.f);
	ASSERT_TRUE(batch.Box(minimum, maximum, glm::vec3(1.f)));
	EXPECT_EQ(batch.GetLineCount(), 12);
	const auto& vertices = batch.GetVertices(true);
	ASSERT_EQ(vertices.size(), 24);
	for (size_t i = 0; i < vertices.size(); i += 2)
	{
		int differing_axes = 0;
		for (int axis = 0; axis < 3; ++axis)
		{
			EXPECT_TRUE(vertices[i].position[axis] == minimum[axis] || vertices[i].position[axis] == maximum[axis]);
			EXPECT_TRUE(vertices[i + 1].position[axis] == minimum[axis] || vertices[i + 1].position[axis] == maximum[axis]);
			if (vertices[i].position[axis] != vertices[i + 1].position[axis]) ++differing_axes;
		}
		EXPECT_EQ(differing_axes, 1);
		for (size_t j = i + 2; j < vertices.size(); j += 2)
			EXPECT_FALSE(vertices[i].position == vertices[j].position && vertices[i + 1].position == vertices[j + 1].position);
	}
}

TEST(DebugDrawBatchTest, ShapeOverflowDoesNotSubmitPartialGeometry)
{
	DebugDrawBatch batch(11);
	EXPECT_FALSE(batch.Box(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	EXPECT_FALSE(batch.Sphere(glm::vec3(0.f), 1.f, glm::vec3(1.f), false, 4));
	EXPECT_EQ(batch.GetLineCount(), 0);
	EXPECT_TRUE(batch.GetVertices(true).empty());
	EXPECT_TRUE(batch.GetVertices(false).empty());
	EXPECT_EQ(batch.GetRejectedPrimitives(), 2);
}

TEST(DebugDrawBatchTest, SphereHasThreeClosedGreatCircles)
{
	DebugDrawBatch batch;
	const glm::vec3 center(2.f, 3.f, 4.f);
	constexpr size_t segments = 8;
	ASSERT_TRUE(batch.Sphere(center, 2.f, glm::vec3(1.f), false, segments));
	const auto& vertices = batch.GetVertices(false);
	ASSERT_EQ(vertices.size(), segments * 6);
	for (const auto& vertex : vertices)
		EXPECT_NEAR(glm::length(vertex.position - center), 2.f, 0.00001f);
	for (size_t circle = 0; circle < 3; ++circle)
	{
		const size_t base = circle * segments * 2;
		for (size_t i = 0; i < segments; ++i)
		{
			EXPECT_EQ(vertices[base + i * 2 + 1].position, vertices[base + ((i + 1) % segments) * 2].position);
			EXPECT_EQ(vertices[base + i * 2].position[static_cast<int>((circle + 2) % 3)], center[static_cast<int>((circle + 2) % 3)]);
		}
	}
}

TEST(DebugDrawBatchTest, InvalidBoundsColorsAndNonfinitePositionsAreRejected)
{
	DebugDrawBatch batch;
	const auto nan = std::numeric_limits<float>::quiet_NaN();
	const auto infinity = std::numeric_limits<float>::infinity();
	EXPECT_FALSE(batch.Line(glm::vec3(nan), glm::vec3(1.f), glm::vec3(1.f)));
	EXPECT_FALSE(batch.Line(glm::vec3(0.f), glm::vec3(infinity), glm::vec3(1.f)));
	EXPECT_FALSE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(-1.f)));
	EXPECT_FALSE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(2.f)));
	EXPECT_FALSE(batch.Box(glm::vec3(1.f), glm::vec3(0.f), glm::vec3(1.f)));
	EXPECT_FALSE(batch.Box(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(nan)));
	EXPECT_EQ(batch.GetLineCount(), 0);
	EXPECT_EQ(batch.GetRejectedPrimitives(), 6);
}

TEST(DebugDrawBatchTest, InvalidSphereRadiiSegmentsAndOverflowAreRejected)
{
	DebugDrawBatch batch;
	for (const auto radius : { 0.f, -1.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
		EXPECT_FALSE(batch.Sphere(glm::vec3(0.f), radius, glm::vec3(1.f)));
	EXPECT_FALSE(batch.Sphere(glm::vec3(0.f), 1.f, glm::vec3(1.f), true, 2));
	EXPECT_FALSE(batch.Sphere(glm::vec3(0.f), 1.f, glm::vec3(1.f), true, 129));
	EXPECT_FALSE(batch.Sphere(glm::vec3(0.f), 1.f, glm::vec3(1.f), true, std::numeric_limits<size_t>::max()));
	EXPECT_FALSE(batch.Sphere(glm::vec3(std::numeric_limits<float>::max()), std::numeric_limits<float>::max(), glm::vec3(1.f)));
	EXPECT_EQ(batch.GetLineCount(), 0);
}

TEST(DebugDrawBatchTest, ClearResetsCountsAndRetainsReservedStorage)
{
	DebugDrawBatch batch(12);
	const auto* depth_storage = batch.GetVertices(true).data();
	const auto* overlay_storage = batch.GetVertices(false).data();
	const auto depth_capacity = batch.GetVertices(true).capacity();
	ASSERT_TRUE(batch.Box(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	EXPECT_FALSE(batch.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	batch.Clear();
	EXPECT_EQ(batch.GetLineCount(), 0);
	EXPECT_EQ(batch.GetRejectedPrimitives(), 0);
	EXPECT_EQ(batch.GetVertices(true).capacity(), depth_capacity);
	EXPECT_EQ(batch.GetVertices(true).data(), depth_storage);
	EXPECT_EQ(batch.GetVertices(false).data(), overlay_storage);
	ASSERT_TRUE(batch.Box(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f), false));
	EXPECT_EQ(batch.GetVertices(false).data(), overlay_storage);
}

TEST(DebugDrawBatchTest, HardLimitBoundsAllocationAndEmptyCapacityRejects)
{
	DebugDrawBatch bounded(std::numeric_limits<size_t>::max());
	EXPECT_EQ(bounded.GetVertices(true).capacity(), DebugDrawBatch::MaxLines * 2);
	DebugDrawBatch empty(0);
	EXPECT_FALSE(empty.Line(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
	EXPECT_EQ(empty.GetRejectedPrimitives(), 1);
}

TEST(DebugDrawBatchTest, ReleaseFacadeNeedsNoRendererOrGraphicsContext)
{
	if constexpr (Daedalus::IsDevBuild())
	{
		GTEST_SKIP() << "Developer facade intentionally requires the active graphics thread";
	}
	else
	{
		EXPECT_FALSE(Daedalus::Renderer::DebugLine(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
		EXPECT_FALSE(Daedalus::Renderer::DebugBox(glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f)));
		EXPECT_FALSE(Daedalus::Renderer::DebugSphere(glm::vec3(0.f), 1.f, glm::vec3(1.f)));
		Daedalus::Renderer::BeginDebugDrawFrame();
		Daedalus::Renderer::FlushDebugDraw();
		EXPECT_EQ(Daedalus::Renderer::GetRejectedDebugPrimitives(), 0);
	}
}
