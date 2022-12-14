#pragma once

#include "Shader.h"

#include <unordered_map>

namespace Daedalus {

class ShaderLibrary 
{
public:
	virtual ~ShaderLibrary() = default;

	void Add(std::shared_ptr<Shader> shader);
	void Remove(const std::string& name);

	std::shared_ptr<Shader> Get(const std::string& name) const;

	bool Exists(const std::string& name) const;
		
protected:
	std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
};

}