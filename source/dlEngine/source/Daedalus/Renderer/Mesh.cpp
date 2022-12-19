#include "dlpch.h"
#include "Mesh.h"

using namespace Daedalus;

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index) :
	m_vertex_count(static_cast<uint32_t>(vertices.size())),
	m_indices_count(static_cast<uint32_t>(indices.size())),
	m_material_index(material_index)
{
	CreateBuffers(vertices, indices);
	ComputeBoundingSphere(vertices);
}

void Mesh::Bind() const
{
	m_VAO->Bind();
}

void Mesh::Unbind() const
{
	m_VAO->Unbind();
}

uint32_t Mesh::GetVertexCount() const
{
	return m_vertex_count;
}

uint32_t Mesh::GetIndexCount() const
{
	return m_indices_count;
}

uint32_t Mesh::GetMaterialIndex() const
{
	return m_material_index;
}

const BoundingSphere Mesh::GetBoundingSphere() const
{
	return m_bounding_sphere;
}

void Mesh::CreateBuffers(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	std::vector<float> vertex_data;
	vertex_data.reserve(vertices.size());

	std::vector<unsigned int> raw_indices;

	for (const auto& vertex : vertices)
	{
		vertex_data.push_back(vertex.position[0]);
		vertex_data.push_back(vertex.position[1]);
		vertex_data.push_back(vertex.position[2]);

		vertex_data.push_back(vertex.tex_coords[0]);
		vertex_data.push_back(vertex.tex_coords[1]);

		vertex_data.push_back(vertex.normals[0]);
		vertex_data.push_back(vertex.normals[1]);
		vertex_data.push_back(vertex.normals[2]);

		vertex_data.push_back(vertex.tangent[0]);
		vertex_data.push_back(vertex.tangent[1]);
		vertex_data.push_back(vertex.tangent[2]);

		vertex_data.push_back(vertex.bitangent[0]);
		vertex_data.push_back(vertex.bitangent[1]);
		vertex_data.push_back(vertex.bitangent[2]);
	}

	m_VAO = VertexArray::Create();
	m_VAO->Bind();

	uint64_t vertex_size = sizeof(Vertex);
	m_VBO = VertexBuffer::Create(vertex_data.data(), vertex_data.size() * sizeof(float));
	m_VBO->SetLayout(BufferLayout
		{ 
			BufferElement{ ShaderDataType::Float3, std::string("v_vertices"), false },
			BufferElement{ ShaderDataType::Float2, std::string("v_texcoord"), false },
			BufferElement{ ShaderDataType::Float3, std::string("v_normals"), false },
			BufferElement{ ShaderDataType::Float3, std::string("v_tangent"), false },
			BufferElement{ ShaderDataType::Float3, std::string("v_bitangent"), false },
		});

	m_EBO = IndexBuffer::Create(indices.data(), indices.size());

	m_VAO->AddVertexBuffer(m_VBO);
	m_VAO->SetIndexBuffer(m_EBO);
	m_VAO->Unbind();
}

void Mesh::ComputeBoundingSphere(const std::vector<Vertex>& vertices)
{

	if (vertices.empty())
		return;

	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();

	float maxX = std::numeric_limits<float>::min();
	float maxY = std::numeric_limits<float>::min();
	float maxZ = std::numeric_limits<float>::min();

	for (const auto& vertex : vertices)
	{
		minX = std::min(minX, vertex.position[0]);
		minY = std::min(minY, vertex.position[1]);
		minZ = std::min(minZ, vertex.position[2]);

		maxX = std::max(maxX, vertex.position[0]);
		maxY = std::max(maxY, vertex.position[1]);
		maxZ = std::max(maxZ, vertex.position[2]);
	}

	m_bounding_sphere.position = glm::vec3{ minX + maxX, minY + maxY, minZ + maxZ } / 2.0f;

	for (const auto& vertex : vertices)
	{
		m_bounding_sphere.radius = std::max(m_bounding_sphere.radius, glm::distance(m_bounding_sphere.position, vertex.position));
	}
}
