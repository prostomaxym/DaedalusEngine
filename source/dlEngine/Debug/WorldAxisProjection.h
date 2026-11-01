#pragma once

#include "Macros.h"
#include <array>
#include <glm/glm.hpp>

namespace Daedalus
{
	class Camera;

	class DAEDALUS_API WorldAxisProjection
	{
	public:
		// Screen-space offsets (Y down), uniformly scaled to fit a unit-radius widget.
		[[nodiscard]] static bool Calculate(const Camera& camera, std::array<glm::vec2, 3>& offsets);
	};
}
