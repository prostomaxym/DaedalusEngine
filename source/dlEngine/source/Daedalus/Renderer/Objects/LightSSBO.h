#pragma once

#include <glm/glm.hpp>

namespace Daedalus
{
	// Should match GLSL memory alignment
	struct LightSSBO
	{
		LightSSBO() = default;
		LightSSBO(int type, glm::vec3 light_pos, glm::vec3 ambient_color, glm::vec3 diffuse_color, glm::vec3 specular_color, float light_power = 1.f,
			float max_distance = 100.f, glm::vec3 direction = {0.f, 0.f, 0.f}, float cutoff = 60.f, float outer_cutoff = 90.f);

													// offset
		glm::vec3 position {0.f, 0.f, 0.f};	
		int align1 = 0;								//0
		glm::vec3 direction {0.f, 0.f, 0.f};		
		int align2 = 0;								//16
		glm::vec3 ambient_color {1.f, 1.f, 1.f};	
		int align3 = 0;								//32
		glm::vec3 diffuse_color {1.f, 1.f, 1.f};	
		int align4 = 0;								//48
		glm::vec3 specular_color {1.f, 1.f, 1.f};	//60
		float power{ 1.f };							//64
		float constant{ 1.f };						//68
		float linear{ 0.045f };						//72
		float quadratic{ 0.0075f };					//76
		float cutoff_angle{ 60.f };					//80
		float outer_cutoff_angle{ 60.f };			//84
		int type = 0;								//88
		int align5 = 0;								//92
		int align6 = 0;								//96

													//TOTAL - 96
		void SetMaxDistance(float distance);
	};
}