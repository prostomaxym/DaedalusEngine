#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSSBO.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API SpotLightSource 
	{
	public:
		SpotLightSource() = default;
		SpotLightSource(glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, 
			float light_power, float max_distance, glm::vec3 direction, float theta_angle, float outer_cutoff);
		void SetCutoffAngle(float theta_angle) { m_params.cutoff_angle = theta_angle; }
		void SetPosition(glm::vec3 position) { m_params.position = position; }
		void SetDirection(glm::vec3 direction) { m_params.direction = direction; }
		void SetPower(float power) { m_params.power = power; }
		void SetMaxDistance(float distance);

		const LightSSBO& GetShaderSSBO() const { return m_params; }

	private:
		LightSSBO m_params{};
		float m_max_distance{ 100.f };
	};
}