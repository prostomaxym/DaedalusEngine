#include "Shader.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

using namespace Daedalus;


std::shared_ptr<Shader> Shader::Create(const std::string& compiled_shader_path)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(compiled_shader_path);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}

std::shared_ptr<Shader> Shader::Create(const std::string& vert_code, const std::string& frag_code, InputType input_type)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(vert_code, frag_code, input_type);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}