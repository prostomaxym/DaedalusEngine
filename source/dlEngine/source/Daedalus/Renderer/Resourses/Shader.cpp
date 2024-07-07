#include "Shader.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

using namespace Daedalus;


std::shared_ptr<Shader> Shader::CreateFromCode(const std::string& code)
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			const auto shader = std::make_shared<OpenGLShader>();
			shader->CreateFromCode(code);
			return shader;
		}
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

std::shared_ptr<Shader> Shader::CreateFromFile(const std::filesystem::path& file_path)
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			const auto shader = std::make_shared<OpenGLShader>();
			shader->CreateFromFile(file_path);
			return shader;
		}
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

std::shared_ptr<Shader> Shader::CreateFromBinary(const std::filesystem::path& binary_path)
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			const auto shader = std::make_shared<OpenGLShader>();
			shader->CreateFromBinary(binary_path);
			return shader;
		}
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return nullptr;
}

std::string Shader::GetSourceFileExtention()
{
	switch (Renderer::GetAPI())
	{
		case RendererAPI::API::None:
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "RendererAPI::None is currently not supported!");
			return {};
		}
		case RendererAPI::API::OpenGL:
		{
			return ".glsl";
		}
	}

	Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unknown RendererAPI!");
	return {};
}

std::string Shader::GetBinaryFileExtention()
{
	return ".dlshader";
}