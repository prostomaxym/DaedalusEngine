#pragma once

#include "Renderer/Objects/Geometry.h"
#include "Renderer/Resourses/Model.h"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Daedalus
{
	// Named GPU textures that passes publish for the following passes (and for debug views)
	enum class PassTexture : uint8_t
	{
		ShadowMap,
		Position,
		Normal,
		Ambient,
		Specular,
		Albedo,
		Shininess,
		SSAO,
		Depth,
		Count
	};

	// Per-frame data shared by all render passes; filled by Renderer, read and extended by passes
	class FrameContext
	{
	public:
		using ModelList = std::vector<std::pair<const Model*, glm::mat4>>;

		const ModelList* frame_models{ nullptr }; // Non-owning, owned by Renderer for the frame duration
		Frustum view_frustum;
		glm::mat4 scene_proj{ 1.f };
		glm::mat4 scene_view{ 1.f };
		int viewport_width{ 0 };
		int viewport_height{ 0 };

		void SetTexture(PassTexture texture, uint32_t id) { m_textures[static_cast<size_t>(texture)] = id; }
		uint32_t GetTexture(PassTexture texture) const { return m_textures[static_cast<size_t>(texture)]; }

	private:
		std::array<uint32_t, static_cast<size_t>(PassTexture::Count)> m_textures{};
	};
}
