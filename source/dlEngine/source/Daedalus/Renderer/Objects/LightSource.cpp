#include "dlpch.h"

#include "LightSource.h"

using namespace Daedalus;

namespace
{
    struct AttentuationParams
    {
        float constant;
        float linear;
        float quadratic;
    };

    std::vector<std::pair<float, AttentuationParams>> lightTable =
    {
        {7.f,    {1.f, 0.7f, 1.8f}},
        {13.f,   {1.f, 0.35f, 0.44f}},
        {20.f,   {1.f, 0.22f, 0.20f}},
        {32.f,   {1.f, 0.14f, 0.07f}},
        {50.f,   {1.f, 0.09f, 0.032f}},
        {65.f,   {1.f, 0.07f, 0.017f}},
        {100.f,  {1.f, 0.045f, 0.0075f}},
        {160.f,  {1.f, 0.027f, 0.0028f}},
        {200.f,  {1.f, 0.022f, 0.0019f}},
        {325.f,  {1.f, 0.014f, 0.0007f}},
        {600.f,  {1.f, 0.007f, 0.0002f}},
        {3250.f, {1.f, 0.0014f, 0.000007f}}
    };

    AttentuationParams GetLightParams(float distance)
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
        float d0 = lightTable[idx].first;
        float d1 = lightTable[idx + 1].first;
        float t = static_cast<float>(distance - d0) / (d1 - d0);
        AttentuationParams params0 = lightTable[idx].second;
        AttentuationParams params1 = lightTable[idx + 1].second;

        AttentuationParams interpolatedParams;
        interpolatedParams.constant = params0.constant + t * (params1.constant - params0.constant);
        interpolatedParams.linear = params0.linear + t * (params1.linear - params0.linear);
        interpolatedParams.quadratic = params0.quadratic + t * (params1.quadratic - params0.quadratic);

        return interpolatedParams;
    }
}

LightSource::LightSource(LightSourceType type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color,
    bool cast_shadows, float light_power, float max_distance, glm::vec3 direction, float cutoff, float outer_cutoff) :
    m_params(type
        , light_pos
        , ambient_color
        , diffuse_color
        , specular_color
        , cast_shadows
        , light_power
        , max_distance
        , direction
        , cutoff
        , outer_cutoff)
    , m_max_distance(max_distance)
{
}

void LightSource::UpdateSSBOForViewFrustum(const glm::mat4& proj, const glm::mat4& view)
{
	m_params.proj_view = CalculateLightMatrixForFrustum(proj, view);
}

void LightSource::SetMaxDistance(float distance)
{
	m_params.SetMaxDistance(distance);
}

LightSSBO::LightSSBO(LightSourceType light_type, glm::vec3 light_pos, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool cast_shadows,
    float light_power, float max_distance, glm::vec3 dir, float cutoff, float outer_cutoff) :
    position(light_pos)
    , direction(dir)
    , ambient_color(ambient)
    , diffuse_color(diffuse)
    , specular_color(specular)
    , power(light_power)
    , cutoff_angle(cutoff)
    , outer_cutoff_angle(outer_cutoff)
    , type(light_type)
    , cast_shadows(cast_shadows ? 1 : 0)
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

float LightSSBO::GetMaxDistance() const
{
    AttentuationParams params {constant, linear, quadratic};
    // Lambda function to calculate the Euclidean distance between two LightParams
    auto calcDistance = [](const AttentuationParams& a, const AttentuationParams& b)
    {
        return std::sqrt(
            (a.constant - b.constant) * (a.constant - b.constant) +
            (a.linear - b.linear) * (a.linear - b.linear) +
            (a.quadratic - b.quadratic) * (a.quadratic - b.quadratic)
        );
    };

    // Start with the first entry as the closest
    float closestDistance = lightTable[0].first;
    float closestDifference = calcDistance(params, lightTable[0].second);

    // Iterate through the lightTable to find the closest match
    for (const auto& entry : lightTable)
    {
        float difference = calcDistance(params, entry.second);
        if (difference < closestDifference)
        {
            closestDistance = entry.first;
            closestDifference = difference;
        }
    }

    return closestDistance;
}