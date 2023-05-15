#include "dlpch.h"
#include "VertexArray.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Daedalus {

	std::shared_ptr<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexArray>();
		}


		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown Renderer API");
		return nullptr;
	}

}