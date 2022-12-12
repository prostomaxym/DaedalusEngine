#include "dlpch.h"
#include "VertexBuffer.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexBuffer.h"


using namespace Daedalus;

std::unique_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLVertexBuffer>(size);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}

std::unique_ptr<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLVertexBuffer>(vertices, size);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}