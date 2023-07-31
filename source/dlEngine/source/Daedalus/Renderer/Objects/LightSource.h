#pragma once

#include "Daedalus/Renderer/Resourses/Shader.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	class DAEDALUS_API LightSource
	{
	public:
		LightSource(std::shared_ptr<Shader> shader) : m_shader(shader) {};
		LightSource(std::shared_ptr<Shader> shader, const glm::vec3& light_pos, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color);
		void SetLightPosition(const glm::vec3& position);

	private:
		std::shared_ptr<Shader> m_shader;
		glm::vec3 m_light_pos{};
		glm::vec3 m_ambient_color{ 1.f, 1.f, 1.f };
		glm::vec3 m_diffuse_color{ 1.f, 1.f, 1.f };
		glm::vec3 m_specular_color{ 1.f, 1.f, 1.f };
	};
}