#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSource.h"
#include "Geometry.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API DirectionalLightSource : public LightSource
	{
	public:
		DirectionalLightSource() = default;
		DirectionalLightSource(const LightProps& props);

		glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const override;
		glm::mat4 CalculateProjViewForSphere(const BoundingSphere& sphere) const;
		glm::mat4 CalculateProjViewForAABB(const AABB& aabb) const;
	};
}