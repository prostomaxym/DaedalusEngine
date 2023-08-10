#pragma once

// Some hardcoded stuff here. Hopefully get rid of this in future
// But for now at least keep them in one place

namespace Daedalus
{
	namespace ShaderConstants
	{
		inline constexpr auto StandardShader{ "Standard" };
		inline constexpr auto CubemapShader{ "Cubemap" };

		inline constexpr auto SceneModel{ "u_object.model_mat" };
		inline constexpr auto ShadowModel{ "u_model" };
		inline constexpr auto ShadowLightSpace{ "u_light_space_matrix" };
		inline constexpr auto ShadowMap{ "u_shadow_map" };

		inline constexpr auto MaterialTexDiffuse{ "u_object.tex_diffuse" };
		inline constexpr auto MaterialTexSpecular{ "u_object.tex_specular" };
		inline constexpr auto MaterialTexNormal{ "u_object.tex_normal" };
		inline constexpr auto MaterialKAmbient{ "u_object.k_ambient" };
		inline constexpr auto MaterialKDiffuse{ "u_object.k_diffuse" };
		inline constexpr auto MaterialKSpecular{ "u_object.k_specular" };
		inline constexpr auto MaterialShininess{ "u_object.shininess" };

		inline constexpr auto ConfigDiffuseMapUsed{ "u_object.enable_diffuse_map" };
		inline constexpr auto ConfigSpecularMapUsed{ "u_object.enable_specular_map" };
		inline constexpr auto ConfigNormalMapUsed{ "u_object.enable_normal_map" };

		inline constexpr auto CubemapTexture{ "u_cubemap" };
		inline constexpr auto CubemapProjectionView{ "u_projection_view" };
	}
}