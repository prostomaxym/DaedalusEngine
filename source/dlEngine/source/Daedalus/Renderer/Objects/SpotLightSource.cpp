#include "dlpch.h"

#include "SpotLightSource.h"

using namespace Daedalus;

SpotLightSource::SpotLightSource(glm::vec3 light_pos
	, glm::vec3 ambient_color
	, glm::vec3 diffuse_color
	, glm::vec3 specular_color
	, float light_power
	, float max_distance
	, glm::vec3 direction
	, float cutoff_angle
	, float outer_cutoff) :
    m_params(2, light_pos, ambient_color, diffuse_color, specular_color, light_power, max_distance, direction, std::cos(glm::radians(cutoff_angle)), std::cos(glm::radians(outer_cutoff)))
	, m_max_distance(max_distance)
{}

void SpotLightSource::SetMaxDistance(float distance)
{
	m_params.SetMaxDistance(distance);
}