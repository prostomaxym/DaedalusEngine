#include "dlpch.h"

#include "LightSource.h"

using namespace Daedalus;

LightSource::LightSource(std::shared_ptr<Shader> shader,
    const glm::vec3& light_pos,
    const glm::vec3& light_color,
    float ambient_strength,
    float spec_strength,
    float shininess_strength,
    float light_power) :
    m_shader(shader)
    , m_light_pos(light_pos)
    , m_light_color(light_color)
    , m_ambient_strength(ambient_strength)
	, m_spec_strength(spec_strength)
	, m_shininess_strength(shininess_strength)
	, m_light_power(light_power)
{
    m_shader->Bind();

	m_shader->SetFloat3("u_light_pos", light_pos);
	m_shader->SetFloat3("u_light_color", light_color);
	m_shader->SetFloat("u_ambient_strength", ambient_strength);
	m_shader->SetFloat("u_spec_strength", spec_strength);
	m_shader->SetFloat("u_light_power", light_power);
	m_shader->SetFloat("u_shininess", shininess_strength);

    m_shader->Unbind();
}

void LightSource::SetLightPosition(const glm::vec3& position)
{
    m_shader->Bind();
    m_shader->SetFloat3("u_light_pos", position);
    m_shader->Unbind();
}

void LightSource::SetLightColor(const glm::vec3& color)
{
    m_shader->Bind();
    m_shader->SetFloat3("u_light_color", color);
    m_shader->Unbind();
}

void LightSource::SetAmbientStrength(float strength)
{
    m_shader->Bind();
    m_shader->SetFloat("u_ambient_strength", strength);
    m_shader->Unbind();
}

void LightSource::SetSpecularStrength(float strength)
{
    m_shader->Bind();
    m_shader->SetFloat("u_spec_strength", strength);
    m_shader->Unbind();
}

void LightSource::SetShininessStrength(float strength)
{
    m_shader->Bind();
    m_shader->SetFloat("u_light_power", strength);
    m_shader->Unbind();
}

void LightSource::SetLightPower(float power)
{
    m_shader->Bind();
    m_shader->SetFloat("u_shininess", power);
    m_shader->Unbind();
}
