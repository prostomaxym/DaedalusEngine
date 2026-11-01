#include "dlpch.h"
#include "DebugDrawBatch.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace Daedalus
{
	namespace
	{
		bool Finite(const glm::vec3& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
		}
		bool ValidColor(const glm::vec3& color)
		{
			return Finite(color) && glm::all(glm::greaterThanEqual(color, glm::vec3(0.f))) &&
				glm::all(glm::lessThanEqual(color, glm::vec3(1.f)));
		}
	}

	DebugDrawBatch::DebugDrawBatch(size_t line_capacity) : m_capacity(std::min(line_capacity, MaxLines))
	{
		for (auto& vertices : m_vertices) vertices.reserve(m_capacity * 2);
	}

	bool DebugDrawBatch::Accept(size_t lines, bool valid)
	{
		if (!valid || lines > m_capacity - m_line_count)
		{
			++m_rejected_primitives;
			return false;
		}
		return true;
	}

	void DebugDrawBatch::Append(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool depth_test)
	{
		auto& vertices = m_vertices[depth_test ? 0 : 1];
		vertices.push_back({ start, color });
		vertices.push_back({ end, color });
		++m_line_count;
	}

	bool DebugDrawBatch::Line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool depth_test)
	{
		if (!Accept(1, Finite(start) && Finite(end) && ValidColor(color))) return false;
		Append(start, end, color, depth_test);
		return true;
	}

	bool DebugDrawBatch::Box(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& color, bool depth_test)
	{
		if (!Accept(12, Finite(minimum) && Finite(maximum) && ValidColor(color) && glm::all(glm::lessThanEqual(minimum, maximum))))
			return false;
		std::array<glm::vec3, 8> corners;
		for (size_t i = 0; i < corners.size(); ++i)
			corners[i] = { (i & 1) ? maximum.x : minimum.x, (i & 2) ? maximum.y : minimum.y, (i & 4) ? maximum.z : minimum.z };
		for (size_t i = 0; i < corners.size(); ++i)
			for (size_t axis = 1; axis <= 4; axis *= 2)
				if (!(i & axis)) Append(corners[i], corners[i | axis], color, depth_test);
		return true;
	}

	bool DebugDrawBatch::Sphere(const glm::vec3& center, float radius, const glm::vec3& color, bool depth_test, size_t segments)
	{
		if (segments < 3 || segments > 128)
		{
			++m_rejected_primitives;
			return false;
		}
		if (!Accept(segments * 3, Finite(center) && std::isfinite(radius) && radius > 0.f && ValidColor(color) &&
			Finite(center + glm::vec3(radius)) && Finite(center - glm::vec3(radius)))) return false;
		for (size_t axis = 0; axis < 3; ++axis)
		{
			const auto point = [&](size_t index)
			{
				const float angle = glm::two_pi<float>() * static_cast<float>(index % segments) / static_cast<float>(segments);
				glm::vec3 offset(0.f);
				offset[static_cast<int>(axis)] = radius * std::cos(angle);
				offset[static_cast<int>((axis + 1) % 3)] = radius * std::sin(angle);
				return center + offset;
			};
			for (size_t i = 0; i < segments; ++i) Append(point(i), point(i + 1), color, depth_test);
		}
		return true;
	}

	void DebugDrawBatch::Clear()
	{
		for (auto& vertices : m_vertices) vertices.clear();
		m_line_count = 0;
		m_rejected_primitives = 0;
	}
}
