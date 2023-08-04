#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>

namespace Daedalus {

	enum class ModelParserFlags : uint32_t
	{
		NONE = 0x0,
		CALC_TANGENT_SPACE = 0x1,
		JOIN_IDENTICAL_VERTICES = 0x2,
		MAKE_LEFT_HANDED = 0x4,
		TRIANGULATE = 0x8,
		REMOVE_COMPONENT = 0x10,
		GEN_NORMALS = 0x20,
		GEN_SMOOTH_NORMALS = 0x40,
		SPLIT_LARGE_MESHES = 0x80,
		PRE_TRANSFORM_VERTICES = 0x100,
		LIMIT_BONE_WEIGHTS = 0x200,
		VALIDATE_DATA_STRUCTURE = 0x400,
		IMPROVE_CACHE_LOCALITY = 0x800,
		REMOVE_REDUNDANT_MATERIALS = 0x1000,
		FIX_INFACING_NORMALS = 0x2000,
		SORT_BY_PTYPE = 0x8000,
		FIND_DEGENERATES = 0x10000,
		FIND_INVALID_DATA = 0x20000,
		GEN_UV_COORDS = 0x40000,
		TRANSFORM_UV_COORDS = 0x80000,
		FIND_INSTANCES = 0x100000,
		OPTIMIZE_MESHES = 0x200000,
		OPTIMIZE_GRAPH = 0x400000,
		FLIP_UVS = 0x800000,
		FLIP_WINDING_ORDER = 0x1000000,
		SPLIT_BY_BONE_COUNT = 0x2000000,
		DEBONE = 0x4000000,
		GLOBAL_SCALE = 0x8000000,
		EMBED_TEXTURES = 0x10000000,
		FORCE_GEN_NORMALS = 0x20000000,
		DROP_NORMALS = 0x40000000,
		GEN_BOUNDING_BOXES = 0x80000000
	};

	inline ModelParserFlags operator~ (ModelParserFlags a) { return (ModelParserFlags)~(int)a; }
	inline ModelParserFlags operator| (ModelParserFlags a, ModelParserFlags b) { return (ModelParserFlags)((int)a | (int)b); }
	inline ModelParserFlags operator& (ModelParserFlags a, ModelParserFlags b) { return (ModelParserFlags)((int)a & (int)b); }
	inline ModelParserFlags operator^ (ModelParserFlags a, ModelParserFlags b) { return (ModelParserFlags)((int)a ^ (int)b); }
	inline ModelParserFlags& operator|= (ModelParserFlags& a, ModelParserFlags b) { return (ModelParserFlags&)((int&)a |= (int)b); }
	inline ModelParserFlags& operator&= (ModelParserFlags& a, ModelParserFlags b) { return (ModelParserFlags&)((int&)a &= (int)b); }
	inline ModelParserFlags& operator^= (ModelParserFlags& a, ModelParserFlags b) { return (ModelParserFlags&)((int&)a ^= (int)b); }

	class DAEDALUS_API Model
	{
	public:
		Model() = default;
		Model(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE);

		Model(const Model& other);
		Model(Model&& other) noexcept;
		Model& operator=(const Model& other);
		Model& operator=(Model&& other) noexcept;

		const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const;
		const std::vector<Material>& GetMaterials() const;
		BoundingSphere GetBoundingSphere() const;

	private:
		void ComputeBoundingSphere();

	private:
		std::vector<std::shared_ptr<Mesh>> m_meshes;
		std::vector<Material> m_material_data;

		BoundingSphere m_bounding_sphere;
	};

	class DAEDALUS_API AssimpParser
	{
	public:
		static bool LoadModel(const std::filesystem::path& fileName
			, std::vector<std::shared_ptr<Mesh>>& meshes
			, std::vector<Material>& material_data
			, ModelParserFlags parser_flags);

	private:
		static void ProcessMaterials(const struct aiScene* scene, std::vector<Material>& material_data, const std::filesystem::path& file_name);
		static void ProcessNode(void* p_transform, struct aiNode* node, const struct aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes);
		static void ProcessMesh(void* p_transform, struct aiMesh* mesh, const struct aiScene* scene, std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices);
	};

}