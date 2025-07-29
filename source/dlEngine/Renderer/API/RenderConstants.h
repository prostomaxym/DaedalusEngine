#pragma once

// Some hardcoded stuff here. Hopefully get rid of this in future
// But for now at least keep them in one place

namespace Daedalus
{
	namespace ShaderConstants
	{
		constexpr auto ForwardShader{ "ForwardPass" };
		constexpr auto DeferredGShader{ "DeferredGeometryPass" };
		constexpr auto DeferredLightShader{ "DeferredLightPass" };
		constexpr auto SSAOShader{ "SSAO" };
		constexpr auto BlurShader{ "Blur" };
		constexpr auto CubemapShader{ "Cubemap" };
		constexpr auto ShadowShader{ "ShadowPass" };
		
		constexpr auto SceneModel{ "u_object.model_mat" };
		constexpr auto ShadowModel{ "u_model" };
		constexpr auto ShadowMaps{ "u_shadowmaps" };

		constexpr auto MaterialTexDiffuse{ "u_object.tex_diffuse" };
		constexpr auto MaterialTexSpecular{ "u_object.tex_specular" };
		constexpr auto MaterialTexNormal{ "u_object.tex_normal" };
		constexpr auto MaterialTexHeight{ "u_object.tex_height" };
		constexpr auto MaterialKAmbient{ "u_object.k_ambient" };
		constexpr auto MaterialKDiffuse{ "u_object.k_diffuse" };
		constexpr auto MaterialKSpecular{ "u_object.k_specular" };
		constexpr auto MaterialShininess{ "u_object.shininess" };

		constexpr auto ConfigDiffuseMapUsed{ "u_object.enable_diffuse_map" };
		constexpr auto ConfigSpecularMapUsed{ "u_object.enable_specular_map" };
		constexpr auto ConfigNormalMapUsed{ "u_object.enable_normal_map" };
		constexpr auto ConfigHeightMapUsed{ "u_object.enable_height_map" };

		constexpr auto CubemapTexture{ "u_cubemap" };
		constexpr auto CubemapProjectionView{ "u_projection_view" };

		constexpr auto SSAOKernel{ "u_samples" };
		constexpr auto SSAOProjection{ "u_projection" };
		constexpr auto SSAOView{ "u_view" };

		constexpr auto VerticesVar{ "vin_vertices" };
		constexpr auto TexCoordVar{ "vin_texcoord" };
		constexpr auto NormalsVar{ "vin_normals" };
		constexpr auto TangentVars{ "vin_tangent" };
		constexpr auto BitangentVars{ "vin_bitangent" };

		constexpr auto GBufferPos{ "u_gPosition" };
		constexpr auto GBufferNorm{ "u_gNormal" };
		constexpr auto GBufferAmbient{ "u_gAmbient" };
		constexpr auto GBufferSpec{ "u_gSpec" };
		constexpr auto GBufferAlbedo{ "u_gAlbedo" };
		constexpr auto GBufferShininess{ "u_gShininess" };
		constexpr auto SSAOBufferNoise{ "u_tex_noise" };
		constexpr auto SSAOBlurBufferNoise{ "u_tex_input" };
		constexpr auto SSAOFinalBuffer{ "u_gSSAO" };
	
		constexpr auto ComputeInput{ "u_input_image" };
		constexpr auto ComputeArray{ "u_input_array" };
		constexpr auto ComputeOutput{ "u_compute_image" };
		constexpr auto LayerIndex{ "u_layer_index" };
		constexpr auto ArrayToGrayScaleShader{ "TextureArrayToGrayScale" };
		constexpr auto TextureToGrayScaleShader{ "TextureToGrayScale" };
	}
}