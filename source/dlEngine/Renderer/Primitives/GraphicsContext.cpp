#include "GraphicsContext.h"

#include "Renderer/API/RendererAPI.h"
#ifdef DL_PLATFORM_ANDROID
#include "Platform/Android/AndroidContext.h"
#else
#include "Platform/OpenGL/OpenGLContext.h"
#endif

using namespace Daedalus;

std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window_handle)
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:
#ifdef DL_PLATFORM_ANDROID
		return std::make_unique<AndroidContext>(static_cast<ANativeWindow*>(window_handle));
#else
		return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window_handle));
#endif
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}
