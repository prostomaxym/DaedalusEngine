#pragma once

#include "Macros.h"
#include "Renderer/Objects/LightSource.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API SpotLightSource : public LightSource
	{
	public:
		SpotLightSource() = default;
		SpotLightSource(const LightProps& props);

		void SetCutoffAngle(float cutoff_cosine)
		{
			m_params.cutoff_angle = cutoff_cosine;
			m_props.theta_angle = glm::degrees(glm::acos(cutoff_cosine));
		}

		glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const override;
	};
}