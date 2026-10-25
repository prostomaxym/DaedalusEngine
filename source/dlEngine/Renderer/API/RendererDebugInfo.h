#pragma once

#include "Renderer/Resourses/Texture.h"

#include <cstdint>
#include <memory>

namespace Daedalus
{
	// Texture ids and views collected by Renderer for the debug overlay
	struct RendererDebugInfo
	{
		struct ShadowTextures
		{
			uint32_t shadow_map_id{ 0 };
		};

		struct GeometryTextures
		{
			uint32_t pos_texture{ 0 };
			uint32_t norm_texture{ 0 };
			uint32_t ambient_texture{ 0 };
			uint32_t spec_texture{ 0 };
			uint32_t albedo_texture{ 0 };
			uint32_t shininess_texture{ 0 };
		};

		struct SSAOTextures
		{
			uint32_t ssao_texture{ 0 };
		};

		ShadowTextures shadow;
		GeometryTextures geom;
		SSAOTextures ssao;
		uint32_t depth_id{ 0 };
		int number_of_shadow_maps{ 0 };
		int current_shadow_map{ 0 };

		std::shared_ptr<Texture2D> greyscale_depth{ nullptr };
		std::shared_ptr<Texture2D> greyscale_shadow{ nullptr };
		std::shared_ptr<Texture2D> greyscale_ssao{ nullptr };
		std::shared_ptr<Texture2D> greyscale_shininess{ nullptr };
	};
}
