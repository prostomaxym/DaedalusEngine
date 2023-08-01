#include "dlpch.h"

#include "LightSource.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

LightSource::LightSource(std::shared_ptr<Shader> shader, const glm::vec3& light_pos, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color, float light_power) :
    m_shader(shader)
    , m_light_pos(light_pos)
    , m_ambient_color(ambient_color)
    , m_diffuse_color(diffuse_color)
	, m_specular_color(specular_color)
    , m_light_power(light_power)
{
    m_shader->Bind();

	m_shader->SetFloat3(ShaderConstants::LightPos, light_pos);
	m_shader->SetFloat3(ShaderConstants::LightAmbient, ambient_color);
	m_shader->SetFloat3(ShaderConstants::LightDiffuse, diffuse_color);
	m_shader->SetFloat3(ShaderConstants::LightSpecular, specular_color);
    m_shader->SetFloat(ShaderConstants::LightPower, light_power);

    m_shader->Unbind();
}

void LightSource::SetLightPosition(const glm::vec3& position)
{
    m_shader->Bind();
    m_shader->SetFloat3(ShaderConstants::LightPos, position);
    m_shader->Unbind();
}
