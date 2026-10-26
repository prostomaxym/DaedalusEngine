#pragma once

#include "Renderer/Objects/Geometry.h"

#include <glm/glm.hpp>

#include <string_view>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Daedalus {

	class ResourceManager;
	class VertexArray;
	class VertexBuffer;
	class IndexBuffer;

	class Mesh
	{
		friend class ResourceManager;
		Mesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index);

	public:
		void Bind() const;
		void Unbind() const;

		std::shared_ptr<VertexArray> GetVertexArray() const { return m_VAO; }

		const std::string& GetName() const { return m_name; }
		uint64_t GetVertexBufferBytes() const { return m_vertex_buffer_bytes; }
		uint64_t GetIndexBufferBytes() const { return static_cast<uint64_t>(m_indices_count) * sizeof(uint32_t); }

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
		std::string m_name;
		const uint32_t m_vertex_count;
		const uint32_t m_indices_count;
		const uint32_t m_material_index;
		uint64_t m_vertex_buffer_bytes{ 0 };

		std::shared_ptr<VertexArray> m_VAO;
		std::shared_ptr<VertexBuffer> m_VBO;
		std::shared_ptr<IndexBuffer> m_EBO;

		BoundingSphere m_bounding_sphere;
		AABB m_bounding_aabb;
	};
}