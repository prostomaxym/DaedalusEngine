#pragma once

#include "Camera.h"
#include "Geometry.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Daedalus {

	struct DAEDALUS_API CameraProjectionProps
	{
		float fov = 70.f;
		float aspect_ratio = 16.f / 9.f;
		float z_near = 0.1f;
		float z_far = 10.f;

		CameraProjectionProps(float FOV = 70.f, float Aspect_ratio = 1.7777f, float Z_near = 0.1f, float Z_far = 10.f) :
			fov(FOV)
			, aspect_ratio(Aspect_ratio)
			, z_near(Z_near)
			, z_far(Z_far)
		{
		}
	};

	class DAEDALUS_API PerspectiveCamera : public Camera
	{
	public:
		explicit PerspectiveCamera(CameraProjectionProps proj_props = CameraProjectionProps(), CameraPositionProps pos_props = CameraPositionProps());

		glm::mat4 GetProjectionMatrix() const override;
		glm::mat4 GetProjectionMatrix(float z_far_limit) const override;
		glm::mat4 GetProjectionMatrix(float z_close_limit, float z_far_limit) const override;

		glm::vec3 GetCascadeDistances(float perc_low, float perc_mid, float perc_high) const override;

		void ProcessZoom(float yoffset) override;

		float GetZNear() const { return m_proj_props.z_near; }
		float GetZFar() const { return m_proj_props.z_far; }

	protected:

		CameraProjectionProps m_proj_props;
	};
}