#include "dlpch.h"
#include "ShaderLibrary.h"

using namespace Daedalus;

void ShaderLibrary::Add(std::shared_ptr<Shader> shader)
{
	m_shaders.insert({ shader->GetName(), shader});
}
void ShaderLibrary::Remove(const std::string& name)
{
	m_shaders.erase(name);
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) const
{
	DL_ASSERT(Exists(name), "Cant find shader " + name + "in ShaderLibrary");
	return m_shaders.at(name);
}

bool ShaderLibrary::Exists(const std::string& name) const
{
	return m_shaders.find(name) != m_shaders.end();
}