#include "GraphicsContext.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"

using namespace Daedalus;

std::unique_ptr<GraphicsContext> GraphicsContext::Create(GLFWwindow* window_handle)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLContext>(window_handle);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}
