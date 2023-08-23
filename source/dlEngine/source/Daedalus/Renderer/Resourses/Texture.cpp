#include "dlpch.h"
#include "Texture.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

using namespace Daedalus;

namespace
{
    std::string empty_str;
}

const std::string& Texture::GetPath() const
{
    return empty_str;
}

std::shared_ptr<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:    Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(width, height);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:	Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(path);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

std::shared_ptr<Texture2D> Texture2D::Create(unsigned char* data, int width, int height, int channels)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:	Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(data, width, height, channels);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

void Texture2D::BindTexture(uint32_t ID, uint32_t slot)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:	Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return;
	case RendererAPI::API::OpenGL:  OpenGLTexture2D::BindTextureImpl(ID, slot); return;
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
}

std::shared_ptr<TextureCubemap> TextureCubemap::Create(std::vector<std::string>& faces)
{
	switch (Renderer::GetAPI())
	{
	case RendererAPI::API::None:	Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!"); return nullptr;
	case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTextureCubemap>(faces);
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}
