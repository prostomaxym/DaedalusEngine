#include "dlpch.h"

#include "DirectionalLightSource.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

DirectionalLightSource::DirectionalLightSource(glm::vec3 light_direction, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, float light_power) :
    m_params(0, light_direction, ambient_color, diffuse_color, specular_color, light_power)
{}