#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

namespace Daedalus {

class OrthographicCamera : public Camera
{
public:
	OrthographicCamera(float left, float right, float bottom, float top, float front, float back,
		CameraPositionProps pos_props = CameraPositionProps());

	void SetProjection(float left, float right, float bottom, float top, float front, float back);

	glm::mat4 GetProjectionMatrix() const override;
	glm::mat4 GetProjectionMatrix(float z_far_limit) const override;

	void ProcessZoom(float yoffset) override;

protected:
	float m_left_plane{ -1.f };
	float m_right_plane{ 1.f };
	float m_bottom_plane{ -1.f };
	float m_top_plane{ 1.f };
	float m_near_plane{ -1.f };
	float m_far_plane{ 1.f };
};

}