#pragma once

#include "Macros.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <vector>

namespace Daedalus
{
	struct DebugDrawVertex
	{
		glm::vec3 position;
		glm::vec3 color;
	};

	class DAEDALUS_API DebugDrawBatch
	{
	public:
		static constexpr size_t MaxLines = 4096;
		explicit DebugDrawBatch(size_t line_capacity = MaxLines);
		DebugDrawBatch(const DebugDrawBatch&) = delete;
		DebugDrawBatch& operator=(const DebugDrawBatch&) = delete;
		DebugDrawBatch(DebugDrawBatch&&) = delete;
		DebugDrawBatch& operator=(DebugDrawBatch&&) = delete;

		[[nodiscard]] bool Line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool depth_test = true);
		[[nodiscard]] bool Box(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& color, bool depth_test = true);
		[[nodiscard]] bool Sphere(const glm::vec3& center, float radius, const glm::vec3& color, bool depth_test = true, size_t segments = 32);
		void Clear();
		[[nodiscard]] const std::vector<DebugDrawVertex>& GetVertices(bool depth_test) const { return m_vertices[depth_test ? 0 : 1]; }
		[[nodiscard]] size_t GetLineCount() const { return m_line_count; }
		[[nodiscard]] size_t GetRejectedPrimitives() const { return m_rejected_primitives; }

	private:
		bool Accept(size_t lines, bool valid);
		void Append(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool depth_test);
		std::array<std::vector<DebugDrawVertex>, 2> m_vertices;
		size_t m_capacity;
		size_t m_line_count{ 0 };
		size_t m_rejected_primitives{ 0 };
	};
}
