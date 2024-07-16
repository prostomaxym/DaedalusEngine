#include "dlpch.h"
#include "Daedalus/Renderer/Primitives/FrameBuffer.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"

using namespace Daedalus;

std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

void Framebuffer::CopyFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return;
	case RendererAPI::API::OpenGL:  OpenGLFramebuffer::CopyFramebufferImpl(src_id, dest_id, width, height); return;
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
}

void Framebuffer::CopyDepthFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return;
	case RendererAPI::API::OpenGL:  OpenGLFramebuffer::CopyDepthFramebufferImpl(src_id, dest_id, width, height); return;
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
}

void Framebuffer::CopyColorFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return;
	case RendererAPI::API::OpenGL:  OpenGLFramebuffer::CopyColorFramebufferImpl(src_id, dest_id, width, height); return;
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
}

void Framebuffer::CopyColorAttachment(unsigned int src_id, unsigned int dest_id, int width, int height, int src_attach_id, int dest_attach_id)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return;
	case RendererAPI::API::OpenGL:  OpenGLFramebuffer::CopyColorAttachmentImpl(src_id, dest_id, width, height, src_attach_id, dest_attach_id); return;
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
}