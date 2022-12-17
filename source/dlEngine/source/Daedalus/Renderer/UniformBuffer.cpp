#include "dlpch.h"
#include "UniformBuffer.h"

#include "daedalus/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"

using namespace Daedalus;

std::shared_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLUniformBuffer>(size, binding);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}
