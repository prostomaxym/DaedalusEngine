#include "dlpch.h"

#include "Cubemap.h"
#include "Renderer/Primitives/VertexArray.h"
#include "Renderer/Primitives/VertexBuffer.h"
#include "Renderer/Primitives/BufferLayout.h"

#include "Renderer/Resources/ResourceManager.h"

using namespace Daedalus;

namespace
{
	constexpr float skybox_vertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	constexpr uint32_t skybox_vertex_count = sizeof(skybox_vertices) / (3 * sizeof(float));
	static_assert(sizeof(skybox_vertices) == 36 * 3 * sizeof(float));
}
Cubemap::Cubemap(const std::vector<std::string>& faces)
{
	m_index_count = skybox_vertex_count;
	m_texture = ResourceManager::LoadCubemap(faces);

	m_VAO = VertexArray::Create();
	m_VAO->Bind();

	m_VBO = VertexBuffer::Create(skybox_vertices, sizeof(skybox_vertices));
	m_VBO->SetLayout(BufferLayout
		{
			BufferElement{ ShaderDataType::Float3, std::string("vin_vertices"), false },
		});

	m_VAO->AddVertexBuffer(m_VBO);
	m_VAO->Unbind();
}
