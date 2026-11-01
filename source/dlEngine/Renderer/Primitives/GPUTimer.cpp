#include "dlpch.h"
#include "GPUTimer.h"

#include "Renderer/API/RendererAPI.h"
#include "Platform/OpenGL/OpenGLGPUTimer.h"

using namespace Daedalus;

std::unique_ptr<GPUTimer> GPUTimer::Create()
{
	switch (RendererAPI::GetAPI())
	{
		case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLGPUTimer>();
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}
