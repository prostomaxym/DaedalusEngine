#pragma once

#include "Daedalus/Renderer/Primitives/IndexBuffer.h"
#include "Daedalus/Renderer/Primitives/VertexArray.h"
#include "Daedalus/Renderer/Primitives/VertexBuffer.h"

#include <glm/glm.hpp>

#include <vector>

namespace Daedalus {

struct Vertex
{
	glm::vec3 position { 0.f,0.f,0.f };
	glm::vec2 tex_coords { 0.f,0.f };
	glm::vec3 normals { 0.f,0.f,0.f };
	glm::vec3 tangent { 0.f,0.f,0.f };
	glm::vec3 bitangent { 0.f,0.f,0.f };
};

struct BoundingSphere
{
	glm::vec3 position{ 0.f,0.f,0.f };
	float radius = 0.f;
};

class Mesh
{
public:
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index);

	void Bind() const;
	void Unbind() const;

	std::shared_ptr<VertexArray> GetVertexArray() const { return m_VAO; }

	uint32_t GetVertexCount() const;
	uint32_t GetIndexCount() const;
	uint32_t GetMaterialIndex() const;
	const BoundingSphere GetBoundingSphere() const;

private:
	void CreateBuffers(const std::vector<Vertex>& p_vertices, const std::vector<uint32_t>& indices);
	void ComputeBoundingSphere(const std::vector<Vertex>& vertices);

private:
	const uint32_t m_vertex_count;
	const uint32_t m_indices_count;
	const uint32_t m_material_index;

	std::shared_ptr<VertexArray> m_VAO;
	std::shared_ptr<VertexBuffer> m_VBO;
	std::shared_ptr<IndexBuffer> m_EBO;

	BoundingSphere m_bounding_sphere;
};

}