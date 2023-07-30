#pragma once

#include "Daedalus/Renderer/Resourses/Shader.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	class DAEDALUS_API LightSource
	{
	public:
		LightSource(std::shared_ptr<Shader> shader) : m_shader(shader) {};
		LightSource(std::shared_ptr<Shader> shader, const glm::vec3& light_pos, const glm::vec3& light_color, float ambient_strength, float spec_strength, float shininess_strength, float light_power);

		void SetLightPosition(const glm::vec3& position);
		void SetLightColor(const glm::vec3& color);
		void SetAmbientStrength(float strength);
		void SetSpecularStrength(float strength);
		void SetShininessStrength(float strength);
		void SetLightPower(float power);

	private:
		std::shared_ptr<Shader> m_shader;
		glm::vec3 m_light_pos{};
		glm::vec3 m_light_color{ 1.f, 1.f, 1.f };
		float m_ambient_strength{ 0.1f };
		float m_spec_strength{ 0.3f };
		float m_shininess_strength{ 16.f };
		float m_light_power{ 0.f };
	};
}