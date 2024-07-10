#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSource.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API SpotLightSource : public LightSource
	{
	public:
		SpotLightSource() = default;
		SpotLightSource(const LightProps& props);

		void SetCutoffAngle(float theta_angle) { m_params.cutoff_angle = theta_angle; }

		glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const override;
	};
}