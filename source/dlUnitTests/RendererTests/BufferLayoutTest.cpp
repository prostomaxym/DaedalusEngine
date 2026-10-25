#include <gtest/gtest.h>

#include <Renderer/Primitives/BufferLayout.h>

using namespace Daedalus;

TEST(BufferLayoutTest, shader_data_type_sizes)
{
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float), 4u);
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float3), 12u);
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Mat3), 36u);
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Mat4), 64u);
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Int4), 16u);
	EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Bool), 1u);
}

TEST(BufferLayoutTest, element_component_counts)
{
	EXPECT_EQ(BufferElement(ShaderDataType::Float, "a").GetComponentCount(), 1u);
	EXPECT_EQ(BufferElement(ShaderDataType::Float2, "a").GetComponentCount(), 2u);
	EXPECT_EQ(BufferElement(ShaderDataType::Mat3, "a").GetComponentCount(), 3u);
	EXPECT_EQ(BufferElement(ShaderDataType::Mat4, "a").GetComponentCount(), 4u);
}

TEST(BufferLayoutTest, layout_computes_offsets_and_stride)
{
	BufferLayout layout = {
		{ ShaderDataType::Float3, "position" },
		{ ShaderDataType::Float2, "uv" },
		{ ShaderDataType::Float4, "color", true }
	};

	const auto& elements = layout.GetElements();
	ASSERT_EQ(elements.size(), 3u);
	EXPECT_EQ(elements[0].offset, 0u);
	EXPECT_EQ(elements[1].offset, 12u);
	EXPECT_EQ(elements[2].offset, 20u);
	EXPECT_TRUE(elements[2].normalized);
	EXPECT_EQ(layout.GetStride(), 36u);
}

TEST(BufferLayoutTest, empty_layout_has_zero_stride)
{
	BufferLayout layout;
	EXPECT_EQ(layout.GetStride(), 0u);
	EXPECT_TRUE(layout.begin() == layout.end());
}
