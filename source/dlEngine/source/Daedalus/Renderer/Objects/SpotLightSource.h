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
		SpotLightSource(glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadows,
			float light_power, float max_distance, glm::vec3 direction, float theta_angle, float outer_cutoff);

		void UpdateSSBODefault();

		void SetCutoffAngle(float theta_angle) { m_params.cutoff_angle = theta_angle; }

	protected:
		glm::mat4 CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const override;
		glm::mat4 CalculateLightMatrixDefault() const;
	};
}