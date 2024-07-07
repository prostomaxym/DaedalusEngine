#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Objects/LightSSBO.h"
#include "Geometry.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	class DAEDALUS_API DirectionalLightSource
	{
	public:
		DirectionalLightSource() = default;
		DirectionalLightSource(glm::vec3 light_direction, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadow, float light_power);

		void UpdateSSBOForViewFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view);
		void UpdateSSBOForSceneFrustum(const BoundingSphere& sphere);

		void SetDirection(glm::vec3 direction) { m_params.position = direction; }
		void SetPower(float lpower) { m_params.power = lpower; }
		void SetShadowMapIndex(int index) { m_params.shadow_map_index = index; }

		const LightSSBO& GetShaderSSBO() const { return m_params; }
		const glm::mat4 GetLightSpaceMatrix() const { return m_params.proj_view; }
		bool CastShadow() const { return m_params.cast_shadows > 0; }

	private:
		glm::mat4 CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const;
		glm::mat4 CalculateLightMatrixForSphere(const BoundingSphere& sphere) const;

		LightSSBO m_params{};
	};
}