#pragma once

// Some hardcoded stuff here. Hopefully get rid of this in future
// But for now at least keep them in one place

namespace Daedalus
{
	namespace ShaderConstants
	{
		inline constexpr auto StandardShader{ "Standard" };

		inline constexpr auto LightPos{ "u_light.position" };
		inline constexpr auto LightAmbient{ "u_light.ambient" };
		inline constexpr auto LightDiffuse{ "u_light.diffuse" };
		inline constexpr auto LightSpecular{ "u_light.specular" };
		inline constexpr auto LightPower{ "u_light.power" };

		inline constexpr auto SceneProjectionView{ "u_scene.projection_view" };
		inline constexpr auto SceneModel{ "u_scene.model" };
		inline constexpr auto SceneViewPos{ "u_scene.view_pos" };

		inline constexpr auto MaterialTexDiffuse{ "u_material.tex_diffuse" };
		inline constexpr auto MaterialTexSpecular{ "u_material.tex_specular" };
		inline constexpr auto MaterialTexNormal{ "u_material.tex_normal" };
		inline constexpr auto MaterialKAmbient{ "u_material.k_ambient" };
		inline constexpr auto MaterialKDiffuse{ "u_material.k_diffuse" };
		inline constexpr auto MaterialKSpecular{ "u_material.k_specular" };
		inline constexpr auto MaterialShininess{ "u_material.shininess" };

		inline constexpr auto ConfigDiffuseMapUsed{ "u_config.diffuse_map_used" };
		inline constexpr auto ConfigSpecularMapUsed{ "u_config.specular_map_used" };
		inline constexpr auto ConfigNormalMapUsed{ "u_config.normal_map_used" };
		inline constexpr auto GammaCorrectionUsed{ "u_config.gamma_correction_used" };
	}
}