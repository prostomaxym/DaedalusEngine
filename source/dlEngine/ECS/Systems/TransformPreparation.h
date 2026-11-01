#pragma once

#include "Macros.h"
#include <glm/mat4x4.hpp>
#include <span>

namespace Daedalus
{
	class JobSystem;
	struct TransformComponent;

	// Inputs are snapshots, not live registry storage. Output elements are independently writable.
	[[nodiscard]] DAEDALUS_API bool PrepareTransforms(JobSystem& jobs,
		std::span<const TransformComponent> inputs, std::span<glm::mat4> outputs);
}
