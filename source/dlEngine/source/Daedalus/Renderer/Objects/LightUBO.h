#pragma once

#include <glm/glm.hpp>

namespace Daedalus
{
	struct LightUBO
	{
		LightUBO() = default;
		LightUBO(int type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, float light_power = 1.f, float max_distance = 100.f, float cutoff = 60.f);

		glm::vec3 position {0.f, 0.f, 0.f}; // for directional light this is direction as we do not care about length of position vector
		glm::vec3 ambient_color {1.f, 1.f, 1.f};
		glm::vec3 diffuse_color {1.f, 1.f, 1.f};
		glm::vec3 specular_color {1.f, 1.f, 1.f};
		float power{ 1.f }; // 0...1 range
		float constant{ 1.f };
		float linear{ 0.045f };
		float quadratic{ 0.0075f };
		float cutoff_angle{ 60.f };
		int type = 0;

		void SetMaxDistance(float distance);
	};
}