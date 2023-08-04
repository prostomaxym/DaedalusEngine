#include "dlpch.h"

#include "LightSSBO.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Daedalus;

namespace
{
    struct LightParams
    {
        float constant;
        float linear;
        float quadratic;
    };

    std::vector<std::pair<int, LightParams>> lightTable =
    {
        {7,    {1.f, 0.7f, 1.8f}},
        {13,   {1.f, 0.35f, 0.44f}},
        {20,   {1.f, 0.22f, 0.20f}},
        {32,   {1.f, 0.14f, 0.07f}},
        {50,   {1.f, 0.09f, 0.032f}},
        {65,   {1.f, 0.07f, 0.017f}},
        {100,  {1.f, 0.045f, 0.0075f}},
        {160,  {1.f, 0.027f, 0.0028f}},
        {200,  {1.f, 0.022f, 0.0019f}},
        {325,  {1.f, 0.014f, 0.0007f}},
        {600,  {1.f, 0.007f, 0.0002f}},
        {3250, {1.f, 0.0014f, 0.000007f}}
    };

    LightParams GetLightParams(int distance)
    {
        // If the distance is smaller than the smallest entry, return the first parameters.
        if (distance <= lightTable[0].first)
        {
            return lightTable[0].second;
        }

        // If the distance is larger than the largest entry, return the last parameters.
        if (distance >= lightTable.back().first)
        {
            return lightTable.back().second;
        }

        // Find the two closest entries for interpolation.
        int idx = 0;
        while (lightTable[idx + 1].first <= distance)
        {
            idx++;
        }

        // Perform linear interpolation.
        int d0 = lightTable[idx].first;
        int d1 = lightTable[idx + 1].first;
        float t = static_cast<float>(distance - d0) / (d1 - d0);
        LightParams params0 = lightTable[idx].second;
        LightParams params1 = lightTable[idx + 1].second;

        LightParams interpolatedParams;
        interpolatedParams.constant = params0.constant + t * (params1.constant - params0.constant);
        interpolatedParams.linear = params0.linear + t * (params1.linear - params0.linear);
        interpolatedParams.quadratic = params0.quadratic + t * (params1.quadratic - params0.quadratic);

        return interpolatedParams;
    }
}

LightSSBO::LightSSBO(int light_type, glm::vec3 light_pos, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float light_power, float max_distance, float cutoff) :
    position(light_pos)
    , ambient_color(ambient)
    , diffuse_color(diffuse)
    , specular_color(specular)
    , power(light_power)
    , cutoff_angle(cutoff)
    , type(light_type)
{
    if (type != 0)
    {
        const auto params = GetLightParams(max_distance);
        constant = params.constant;
        linear = params.linear;
        quadratic = params.quadratic;
    }
}

void LightSSBO::SetMaxDistance(float distance)
{
    const auto params = GetLightParams(distance);
    constant = params.constant;
    linear = params.linear;
    quadratic = params.quadratic;
}
