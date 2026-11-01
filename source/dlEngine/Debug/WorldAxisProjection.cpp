#include "dlpch.h"
#include "WorldAxisProjection.h"
#include "Renderer/Objects/Camera.h"
#include <algorithm>
#include <cmath>

namespace Daedalus
{
	bool WorldAxisProjection::Calculate(const Camera& camera, std::array<glm::vec2, 3>& offsets)
	{
		offsets = {};
		const float near_plane = camera.GetNearPlane();
		const float far_plane = camera.GetFarPlane();
		if (!std::isfinite(near_plane) || !std::isfinite(far_plane) || far_plane <= near_plane) return false;
		const auto projection = camera.GetProjectionMatrix();
		const float distance = near_plane + std::min(5.f, (far_plane - near_plane) * 0.5f);
		const float projection_scale = std::abs(projection[3][3]) < 0.0001f ? std::abs(distance) : 1.f;
		const float half_width = projection_scale / std::abs(projection[0][0]);
		const float half_height = projection_scale / std::abs(projection[1][1]);
		const float depth_margin = std::min(distance - near_plane, far_plane - distance);
		const float size = std::min({ half_width, half_height, depth_margin }) * 0.12f;
		const float aspect = std::abs(projection[1][1] / projection[0][0]);
		if (!std::isfinite(size) || size <= 0.f || !std::isfinite(aspect) || aspect <= 0.f) return false;

		// Match the sandbox smoke axes, including the real third-person view transform.
		const auto origin = camera.GetPosition() + camera.GetDirection() * distance - camera.GetUp() * (size * 1.6f);
		const auto transform = projection * camera.GetViewMatrix();
		const auto project = [&](const glm::vec3& point, glm::vec2& screen)
		{
			const auto clip = transform * glm::vec4(point, 1.f);
			if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) ||
				!std::isfinite(clip.w) || clip.w <= 0.f) return false;
			// Do not visualize geometry that the renderer would clip in depth.
			if (clip.z < -clip.w || clip.z > clip.w) return false;
			screen = { clip.x / clip.w * aspect, -clip.y / clip.w };
			return std::isfinite(screen.x) && std::isfinite(screen.y);
		};

		glm::vec2 screen_origin;
		if (!project(origin, screen_origin)) return false;
		std::array<glm::vec2, 3> projected;
		float radius = 0.f;
		for (size_t axis = 0; axis < projected.size(); ++axis)
		{
			auto endpoint = origin;
			endpoint[static_cast<glm::length_t>(axis)] += size;
			if (!project(endpoint, projected[axis])) return false;
			projected[axis] -= screen_origin;
			radius = std::max(radius, glm::length(projected[axis]));
		}
		if (!std::isfinite(radius) || radius <= 0.f) return false;
		for (size_t axis = 0; axis < projected.size(); ++axis)
			offsets[axis] = projected[axis] / radius;
		return true;
	}
}
