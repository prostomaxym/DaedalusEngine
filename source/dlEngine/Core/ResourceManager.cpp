#include "dlpch.h"

#include "ResourceManager.h"

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> ResourceManager::s_shader_library = nullptr;
std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::s_models = {};
std::unordered_map<std::string, std::shared_ptr<Material>> ResourceManager::s_material = {};
std::unordered_map<std::string, std::shared_ptr<Texture2D>> ResourceManager::s_named_textures = {};
std::set<std::shared_ptr<Texture2D>> ResourceManager::s_unnamed_textures = {};
std::set<std::shared_ptr<TextureCubemap>> ResourceManager::s_unnamed_cubemaps = {};
std::set<std::shared_ptr<Mesh>> ResourceManager::s_meshes = {};
int ResourceManager::s_vertex_count = 0;

void ResourceManager::Init()
{

}

void ResourceManager::Shutdown()
{
	s_shader_library.reset();
	s_models.clear();
	s_material.clear();
	s_named_textures.clear();
	s_unnamed_textures.clear();
	s_unnamed_cubemaps.clear();
	s_meshes.clear();

	s_vertex_count = 0;
}

void ResourceManager::LoadShaders(const std::filesystem::path& path, bool recompile)
{
	s_shader_library = std::make_unique<ShaderLibrary>(path, recompile);
}

std::shared_ptr<Shader> ResourceManager::GetShader(std::string_view name)
{
	return s_shader_library->Get(name.data());
}


std::shared_ptr<Model> ResourceManager::LoadModel(const std::filesystem::path& path, ModelParserFlags parser_flags)
{
	const auto name = path.string();
	auto it = s_models.find(name);

	if (it == s_models.end())
	{
		auto model = std::shared_ptr<Model>(new Model(path, parser_flags));
		s_models[name] = model;

		return model;
	}
	else
	{
		return it->second;
	}
}

std::shared_ptr<Material> ResourceManager::LoadMaterial(std::string_view material_name, glm::vec3 kambient, glm::vec3 kdiffuse,
	glm::vec3 kspecular, float shininess)
{
	std::string key(material_name);
	auto it = s_material.find(key);

	if (it == s_material.end())
	{
		auto material = std::shared_ptr<Material>(new Material(key, kambient, kdiffuse, kspecular, shininess));
		s_material[key] = material;
		return material;
	}
	else
	{
		return it->second;
	}
}

std::shared_ptr<Mesh> ResourceManager::LoadMesh(std::string_view name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t material_index)
{
	auto mesh = std::shared_ptr<Mesh>(new Mesh(name, vertices, indices, material_index));
	s_meshes.insert(mesh);

	s_vertex_count += mesh->GetVertexCount();

	return mesh;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(const std::filesystem::path& path)
{
	const auto name = path.string();
	auto it = s_named_textures.find(name);

	if (it == s_named_textures.end())
	{
		auto texture = Texture2D::Create(path);
		s_named_textures[name] = texture;

		return texture;
	}
	else
	{
		return it->second;
	}
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(uint32_t width, uint32_t height, int channels)
{
	auto texture = Texture2D::Create(width, height, channels);
	s_unnamed_textures.insert(texture);

	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(unsigned char* data, int width, int height, int channels)
{
	auto texture = Texture2D::Create(data, width, height, channels);
	s_unnamed_textures.insert(texture);

	return texture;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(float* data, int width, int height, int channels)
{
	auto texture = Texture2D::Create(data, width, height, channels);
	s_unnamed_textures.insert(texture);

	return texture;
}

std::shared_ptr<TextureCubemap> ResourceManager::LoadCubemap(const std::vector<std::string>& faces)
{
	auto cubemap = TextureCubemap::Create(faces);
	s_unnamed_cubemaps.insert(cubemap);

	return cubemap;
}