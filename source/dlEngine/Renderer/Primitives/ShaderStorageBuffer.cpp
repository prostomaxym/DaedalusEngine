#include "dlpch.h"
#include "ShaderStorageBuffer.h"

#include "Renderer/API/RendererAPI.h"
#include "Platform/OpenGL/OpenGLShaderStorageBuffer.h"

using namespace Daedalus;

std::shared_ptr<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32_t size, uint32_t binding, Type type)
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLShaderStorageBuffer>(size, binding, type);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}
