#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightUBO.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API DirectionalLightSource
	{
	public:
		DirectionalLightSource() = default;
		DirectionalLightSource(glm::vec3 light_direction, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, float light_power);

		void SetDirection(glm::vec3 direction) { m_params.position = direction; }
		void SetPower(float lpower) { m_params.power = lpower; }

		const LightUBO& GetShaderUBO() const { return m_params; }

	private:
		LightUBO m_params{};
	};
}