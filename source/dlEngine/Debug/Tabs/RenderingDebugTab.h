#pragma once

#include <cstdint>
#include <string_view>

struct ImVec2;

namespace Daedalus
{
	class RenderingDebugTab
	{
	public:
		void Render();

	private:
		void RenderTexture(std::string_view name, uint32_t texture_id, ImVec2 section_size, bool use_aspect = true);
	};
}
