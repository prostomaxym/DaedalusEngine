#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSSBO.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API DirectionalLightSource
	{
	public:
		DirectionalLightSource() = default;
		DirectionalLightSource(glm::vec3 light_direction, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadow, float light_power);

		void SetDirection(glm::vec3 direction) { m_params.position = direction; }
		void SetPower(float lpower) { m_params.power = lpower; }

		const LightSSBO& GetShaderSSBO() const { return m_params; }
		const LightSSBO& GetShaderSSBO(const glm::mat4& proj, const glm::mat4& view);
		bool CastShadow() const { return m_params.cast_shadows > 0; }

		glm::mat4 CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view);

	private:
		LightSSBO m_params{};
	};
}