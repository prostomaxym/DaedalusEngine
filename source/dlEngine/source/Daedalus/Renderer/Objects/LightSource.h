#pragma once

#include "Daedalus/Core/Core.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Daedalus
{
	enum LightSourceType
	{
		Directional = 0,
		Point = 1,
		Spot = 2
	};

	// Should match GLSL memory alignment
	struct LightSSBO
	{
		LightSSBO() = default;
		LightSSBO(LightSourceType type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadows, float light_power = 1.f,
			float max_distance = 100.f, glm::vec3 direction = { 0.f, 0.f, 0.f }, float cutoff = 60.f, float outer_cutoff = 90.f);

		glm::mat4 proj_view = glm::mat4(0.0);
		glm::vec3 position = glm::vec3(0.0);
		int align1 = 0;
		glm::vec3 direction = glm::vec3(0.0);
		int align2 = 0;
		glm::vec3 ambient_color = glm::vec3(0.0);
		int align3 = 0;
		glm::vec3 diffuse_color = glm::vec3(0.0);
		int align4 = 0;
		glm::vec3 specular_color = glm::vec3(0.0);
		float power = 1.f;
		float constant = 1.f;
		float linear = 0.045f;
		float quadratic = 0.0075f;
		float cutoff_angle = 60.f;
		float outer_cutoff_angle = 60.f;
		LightSourceType type = LightSourceType::Directional;
		int cast_shadows = 0;
		int shadow_map_index = -1;

		void SetMaxDistance(float distance);
		float GetMaxDistance() const;
	};

class DAEDALUS_API LightSource
	{
	public:
	    LightSource() = default;
		LightSource(LightSourceType type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadows, float light_power,
			float max_distance = 100.f, glm::vec3 direction = { 0.f, 0.f, 0.f }, float cutoff = 60.f, float outer_cutoff = 90.f);

		virtual ~LightSource() = default;

	    void UpdateSSBOForViewFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view);

	    void SetDirection(glm::vec3 direction) { m_params.direction = direction; }
	    void SetPower(float lpower) { m_params.power = lpower; }
	    void SetShadowMapIndex(int index) { m_params.shadow_map_index = index; }
		void SetPosition(glm::vec3 position) { m_params.position = position; }
		void SetMaxDistance(float distance);

	    const LightSSBO& GetShaderSSBO() const { return m_params; }
	    const glm::mat4 GetLightSpaceMatrix() const { return m_params.proj_view; }
	    bool CastShadow() const { return m_params.cast_shadows > 0; }

	protected:
	    virtual glm::mat4 CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const = 0;

	    LightSSBO m_params{};
		float m_max_distance{ 0.f };
	};
}