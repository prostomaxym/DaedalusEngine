#include "dlpch.h"

#include "Material.h"

using namespace Daedalus;

Material::Material(const std::string& material_name, glm::vec3 kambient, glm::vec3 kdiffuse, glm::vec3 kspecular, float shininess) :
	m_material_name(material_name)
	, m_k_ambient(kambient)
	, m_k_diffuse(kdiffuse)
	, m_k_specular(kspecular)
	, m_n_shininess(shininess)
{
}

void Material::SetDiffuseMap(unsigned char* data, int width, int height, int channels)
{
	m_map_diffuse = Texture2D::Create(data, width, height, channels);
}

void Material::SetSpecularMap(unsigned char* data, int width, int height, int channels)
{
	m_map_specular = Texture2D::Create(data, width, height, channels);
}

void Material::SetNormalMap(unsigned char* data, int width, int height, int channels)
{
	m_map_normal = Texture2D::Create(data, width, height, channels);
}