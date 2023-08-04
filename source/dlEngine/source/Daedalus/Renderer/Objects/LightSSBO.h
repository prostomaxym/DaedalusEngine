#pragma once

#include <glm/glm.hpp>

namespace Daedalus
{
	// Should match GLSL memory alignment
	struct LightSSBO
	{
		LightSSBO() = default;
		LightSSBO(int type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, float light_power = 1.f, float max_distance = 100.f, float cutoff = 60.f);

													// offset
		glm::vec3 position {0.f, 0.f, 0.f};			//0
		int align1 = 0;
		glm::vec3 ambient_color {1.f, 1.f, 1.f};	//16
		int align2 = 0;
		glm::vec3 diffuse_color {1.f, 1.f, 1.f};	//32
		int align3 = 0;
		glm::vec3 specular_color {1.f, 1.f, 1.f};	//48
		float power{ 1.f };							//60
		float constant{ 1.f };						//64
		float linear{ 0.045f };						//68
		float quadratic{ 0.0075f };					//72
		float cutoff_angle{ 60.f };					//76
		int type = 0;								//80
		int align4 = 0;								//84
		int align5 = 0;								//88
		int align6 = 0;								//92

													//TOTAL - 96

		void SetMaxDistance(float distance);
	};
}