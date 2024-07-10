#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSource.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	class DAEDALUS_API PointLightSource : public LightSource
	{
	public:
		PointLightSource() = default;
		PointLightSource(glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color,
			bool cast_shadows, float light_power, float max_distance);
	protected:
		virtual glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
		{
			return glm::mat4();
		};
	};
}
