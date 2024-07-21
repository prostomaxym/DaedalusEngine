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

	struct DAEDALUS_API LightProps
	{
		glm::vec3 light_pos{ 0.f, 0.f, 0.f };
		glm::vec3 ambient_color{ 0.f, 0.f, 0.f };
		glm::vec3 diffuse_color{ 0.f, 0.f, 0.f };
		glm::vec3 specular_color{ 0.f, 0.f, 0.f };
		glm::vec3 direction{ 0.f, 0.f, 0.f };
		float light_power{ 0.f };
		float max_distance{ 100.f };
		float theta_angle{ 60.f };;
		float outer_cutoff{ 90.f };
		bool cast_shadows{ false };
		int number_of_shadow_cascades{ 1 };
	};

	// Should match GLSL memory alignment
	struct LightSSBO
	{
		LightSSBO() = default;
		LightSSBO(LightSourceType type, const LightProps& props);

		// Should be aligned to N * 16 bytes
		// Align matrices and vectors with scalars for reducing space
		glm::vec3 position = glm::vec3(0.0);
		LightSourceType type = LightSourceType::Directional;
		glm::vec3 direction = glm::vec3(0.0);
		int cast_shadows = 0;
		glm::vec3 ambient_color = glm::vec3(0.0);
		int shadow_map_index = -1;
		glm::vec3 diffuse_color = glm::vec3(0.0);
		int number_of_cascades = 1;
		glm::vec3 specular_color = glm::vec3(0.0);
		float power = 1.f;
		float constant = 1.f;
		float linear = 0.045f;
		float quadratic = 0.0075f;
		float cutoff_angle = 60.f; // 96 bytes
		float outer_cutoff_angle = 60.f;
		int align1 = 0;
		int align2 = 0;
		int align3 = 0;
		// Total 112 bytes

		void SetMaxDistance(float distance);
		float GetMaxDistance() const;
	};

	class DAEDALUS_API LightSource
	{
	public:
		LightSource() = default;
		LightSource(LightSourceType type, const LightProps& props);

		virtual ~LightSource() = default;

		std::vector<glm::mat4> CalculateCascadesProjView(const glm::mat4& proj, const glm::mat4& view) const;
		virtual glm::mat4 CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const = 0;

		void SetDirection(glm::vec3 direction) { m_params.direction = direction; }
		void SetPower(float lpower) { m_params.power = lpower; }
		void SetShadowMapIndex(int index) { m_params.shadow_map_index = index; }
		void SetShadowNumberOfCascades(int num) { m_params.number_of_cascades = num; }
		void SetPosition(glm::vec3 position) { m_params.position = position; }
		void SetMaxDistance(float distance);

		const LightSSBO& GetShaderSSBO() const { return m_params; }
		virtual int GetShadowNumberOfCascades() const { return m_params.number_of_cascades; }
		bool CastShadow() const { return m_params.cast_shadows > 0; }

	protected:
		void ExtractPerspectiveParams(const glm::mat4& projMatrix, float& fov, float& aspect, float& z_near, float& z_far) const;

		LightSSBO m_params{};;
	};
}