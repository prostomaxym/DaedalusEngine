#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSSBO.h"

#include "Geometry.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API SpotLightSource 
	{
	public:
		SpotLightSource() = default;
		SpotLightSource(glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadows,
			float light_power, float max_distance, glm::vec3 direction, float theta_angle, float outer_cutoff);

		void UpdateSSBOForViewFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view);
		void UpdateSSBODefault();

		void SetCutoffAngle(float theta_angle) { m_params.cutoff_angle = theta_angle; }
		void SetPosition(glm::vec3 position) { m_params.position = position; }
		void SetDirection(glm::vec3 direction) { m_params.direction = direction; }
		void SetPower(float power) { m_params.power = power; }
		void SetMaxDistance(float distance);
		void SetShadowMapIndex(int index) { m_params.shadow_map_index = index; }

		const LightSSBO& GetShaderSSBO() const { return m_params; }
		const glm::mat4 GetLightSpaceMatrix() const { return m_params.proj_view; }
		bool CastShadow() const { return m_params.cast_shadows > 0; }

	private:
		glm::mat4 CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const;
		glm::mat4 CalculateLightMatrixDefault() const;

		LightSSBO m_params{};
		float m_max_distance{ 100.f };
	};
}