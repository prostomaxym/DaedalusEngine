#pragma once

#include <glm/glm.hpp>

namespace Daedalus
{
	// Should match GLSL memory alignment
	struct LightSSBO
	{
		LightSSBO() = default;
		LightSSBO(int type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, bool cast_shadows, float light_power = 1.f,
			float max_distance = 100.f, glm::vec3 direction = { 0.f, 0.f, 0.f }, float cutoff = 60.f, float outer_cutoff = 90.f);

        glm::mat4 proj_view = glm::mat4(0.0);
        glm::vec3 position = glm::vec3(0.0);
        int align1 = 0;
        glm::vec3 direction = glm::vec3(0.0);
        int align2 = 0;
        glm::vec3 ambient_color = glm::vec3(0.0);
        int align3 = 0;
        glm::vec3 diffuse_color = glm::vec3(0.0);
        int align4 = 0;
        glm::vec3 specular_color = glm::vec3(0.0);
        float power = 1.f;
        float constant = 1.f;
        float linear = 0.045f;
        float quadratic = 0.0075f;
        float cutoff_angle = 60.f;
        float outer_cutoff_angle = 60.f;
        int type = 0;
        int cast_shadows = 0;
        int align5 = 0;

		void SetMaxDistance(float distance);
	};
}