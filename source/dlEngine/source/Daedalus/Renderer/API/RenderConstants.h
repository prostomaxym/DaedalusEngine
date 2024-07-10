#pragma once

// Some hardcoded stuff here. Hopefully get rid of this in future
// But for now at least keep them in one place

namespace Daedalus
{
	namespace ShaderConstants
	{
		constexpr auto StandardShader{ "Standard" };
		constexpr auto CubemapShader{ "Cubemap" };
		constexpr auto ShadowShader{ "Shadow" };

		constexpr auto SceneModel{ "u_object.model_mat" };
		constexpr auto ShadowModel{ "u_model" };
		constexpr auto ShadowMaps{ "u_shadowmaps" };

		constexpr auto MaterialTexDiffuse{ "u_object.tex_diffuse" };
		constexpr auto MaterialTexSpecular{ "u_object.tex_specular" };
		constexpr auto MaterialTexNormal{ "u_object.tex_normal" };
		constexpr auto MaterialKAmbient{ "u_object.k_ambient" };
		constexpr auto MaterialKDiffuse{ "u_object.k_diffuse" };
		constexpr auto MaterialKSpecular{ "u_object.k_specular" };
		constexpr auto MaterialShininess{ "u_object.shininess" };

		constexpr auto ConfigDiffuseMapUsed{ "u_object.enable_diffuse_map" };
		constexpr auto ConfigSpecularMapUsed{ "u_object.enable_specular_map" };
		constexpr auto ConfigNormalMapUsed{ "u_object.enable_normal_map" };

		constexpr auto CubemapTexture{ "u_cubemap" };
		constexpr auto CubemapProjectionView{ "u_projection_view" };

		constexpr auto VerticesVar{ "vin_vertices" };
		constexpr auto TexCoordVar{ "vin_texcoord" };
		constexpr auto NormalsVar{ "vin_normals" };
		constexpr auto TangentVars{ "vin_tangent" };
		constexpr auto BitangentVars{ "vin_bitangent" };
	}
}