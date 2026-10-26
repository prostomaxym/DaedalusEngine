#pragma once

#include "Macros.h"
#include "Renderer/Objects/LightSource.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	struct BoundingSphere;
	struct AABB;
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