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

protected:
	float m_left{ -1.f };
	float m_right{ 1.f };
	float m_bottom{ -1.f };
	float m_top{ 1.f };
	float m_front{ -1.f };
	float m_back{ 1.f };
};

}