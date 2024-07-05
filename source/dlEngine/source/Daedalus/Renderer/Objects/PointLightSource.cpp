#include "dlpch.h"

#include "PointLightSource.h"

using namespace Daedalus;

PointLightSource::PointLightSource(glm::vec3 light_pos
	, glm::vec3 ambient_color
	, glm::vec3 diffuse_color
	, glm::vec3 specular_color
	, bool cast_shadows
	, float light_power
	, float max_distance) 
	: 
	m_params(1, light_pos, ambient_color, diffuse_color, specular_color, cast_shadows, light_power, max_distance)
	, m_max_distance(max_distance)
{}

void PointLightSource::SetMaxDistance(float distance)
{
	m_params.SetMaxDistance(distance);
}