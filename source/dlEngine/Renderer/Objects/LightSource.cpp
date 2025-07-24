#include "dlpch.h"

#include "LightSource.h"

#include "Config/GraphicsConfig.h"

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

    glm::mat4 UpdateNearFar(const glm::mat4& projection, float newNearPlane, float newFarPlane)
    {
        float fov = 2.0f * std::atan(1.0f / projection[1][1]);
        float aspectRatio = projection[1][1] / projection[0][0];

        return glm::perspective(fov, aspectRatio, newNearPlane, newFarPlane);
    }

    std::pair<float, float> ExtractNearFar(const glm::mat4& projection)
    {
        return { projection[3][2] / (projection[2][2] - 1.0f), projection[3][2] / (projection[2][2] + 1.0f) };
    }

    float CalculateCascadePlaneDistance(float znear, float zfar, int cascade_plane_number, int number_of_cascades)
    {
        float percent = static_cast<float>(cascade_plane_number) / static_cast<float>(number_of_cascades);
        const auto cascade_exponent = GraphicsConfig::GetShadowCSMExponent();
        return znear + (zfar - znear) * std::pow(percent, cascade_exponent);
    }
}

LightSource::LightSource(LightSourceType type, const LightProps& props) :
    m_params(type, props)
{
}

std::vector<glm::mat4> LightSource::CalculateCascadesProjView(const glm::mat4& proj, const glm::mat4& view) const
{
	std::vector<glm::mat4> light_proj_view;
	light_proj_view.reserve(m_params.number_of_cascades);

    const auto [original_near, original_far] = ExtractNearFar(proj);
	for (auto i = 0; i < m_params.number_of_cascades; ++i)
	{
        const auto new_znear = CalculateCascadePlaneDistance(original_near, original_far, i, m_params.number_of_cascades);
        const auto new_zfar = CalculateCascadePlaneDistance(original_near, original_far, i + 1, m_params.number_of_cascades);
        const auto proj_clipped = UpdateNearFar(proj, new_znear, new_zfar);
		light_proj_view.push_back(CalculateProjViewForFrustum(proj_clipped, view));
	}

	return light_proj_view;
}

void LightSource::SetMaxDistance(float distance)
{
	m_params.SetMaxDistance(distance);
}

LightSSBO::LightSSBO(LightSourceType light_type, const LightProps& props) :
    position(props.light_pos)
    , direction(props.direction)
    , ambient_color(props.ambient_color)
    , diffuse_color(props.diffuse_color)
    , specular_color(props.specular_color)
    , power(props.light_power)
    , cutoff_angle(std::cos(glm::radians(props.theta_angle)))
    , outer_cutoff_angle(std::cos(glm::radians(props.outer_cutoff)))
    , type(light_type)
    , cast_shadows(props.cast_shadows ? 1 : 0)
    , number_of_cascades(props.number_of_shadow_cascades)
{
    if (type != LightSourceType::Directional)
    {
        const auto params = GetLightParams(props.max_distance);
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

// Stupid ChatGPT gave me this shitty code. No time to clean, sry
void LightSource::ExtractPerspectiveParams(const glm::mat4& projMatrix, float& fov, float& aspect, float& z_near, float& z_far) const
{
    aspect = projMatrix[1][1] / projMatrix[0][0];
    fov = 2.0f * atan(1.0f / projMatrix[1][1]);
    z_near = (projMatrix[3][2]) / (projMatrix[2][2] - 1.0f);
    z_far = (projMatrix[3][2]) / (projMatrix[2][2] + 1.0f);
}