#include "dlpch.h"
#include "Daedalus/Renderer/Framebuffer.h"

#include "Daedalus/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLFramebuffer.h"

using namespace Daedalus;
	
std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:    DL_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
	}

	DL_CORE_ASSERT(false, "Unknown RendererAPI!");
	return nullptr;
}

