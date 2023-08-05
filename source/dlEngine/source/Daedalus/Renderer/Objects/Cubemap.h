#pragma once

#include "Daedalus/Renderer/Resourses/Texture.h"
#include "Daedalus/Renderer/Primitives/VertexArray.h"
#include "Daedalus/Renderer/Primitives/VertexBuffer.h"

#include <string>
#include <vector>

namespace Daedalus
{
	class DAEDALUS_API Cubemap
	{
	public:
		Cubemap(std::vector<std::string>& faces);

		const std::shared_ptr<VertexArray>& GetVertexArray() const { return m_VAO; }
		const std::shared_ptr<TextureCubemap>& GetCubemapTexture() const { return m_texture; }

		int GetIndexCount() const { return m_index_count; }

	private:
		std::shared_ptr<TextureCubemap> m_texture;
		std::shared_ptr<VertexArray> m_VAO;
		std::shared_ptr<VertexBuffer> m_VBO;

		uint32_t m_index_count{ 0 };
	};
}