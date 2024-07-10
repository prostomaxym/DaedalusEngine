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
		PointLightSource(const LightProps& props);
	protected:
		virtual glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
		{
			return glm::mat4();
		};
	};
}
