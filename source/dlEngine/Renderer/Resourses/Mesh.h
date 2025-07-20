#pragma once

#include "Renderer/Objects/Geometry.h"
#include "Renderer/Primitives/IndexBuffer.h"
#include "Renderer/Primitives/VertexArray.h"
#include "Renderer/Primitives/VertexBuffer.h"

#include <glm/glm.hpp>

#include <vector>

namespace Daedalus {

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
		BoundingSphere GetBoundingSphere() const;
		AABB GetBoundingAABB() const;

		bool IsVisible(const Frustum& frust, const glm::mat4& transform) const;

	private:
		void CreateBuffers(const std::vector<Vertex>& p_vertices, const std::vector<uint32_t>& indices);
		void ComputeBoundingSphere(const std::vector<Vertex>& vertices);
		void ComputeBoundingAABB(const std::vector<Vertex>& vertices);

	private:
		const uint32_t m_vertex_count;
		const uint32_t m_indices_count;
		const uint32_t m_material_index;

		std::shared_ptr<VertexArray> m_VAO;
		std::shared_ptr<VertexBuffer> m_VBO;
		std::shared_ptr<IndexBuffer> m_EBO;

		BoundingSphere m_bounding_sphere;
		AABB m_bounding_aabb;
	};
}