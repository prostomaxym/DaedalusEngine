#pragma once

#include "Macros.h"
#include "Renderer/Objects/LightSource.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	class DAEDALUS_API PointLightSource : public LightSource
	{
	public:
		PointLightSource() = default;
		PointLightSource(const LightProps& props);

		virtual std::vector<glm::mat4> CalculateCascadesProjView(const glm::mat4& proj, const glm::mat4& view) const override;

		virtual int GetShadowNumberOfCascades() const override { return 6; }

	protected:
		virtual glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const override
		{
			//DL_ASSERT(false, "Not Implemented");
			return glm::mat4();
		}
	};
}
