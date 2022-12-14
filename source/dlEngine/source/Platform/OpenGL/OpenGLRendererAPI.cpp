#include "dlpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/OpenGL/OpenGLShaderLibrary.h"

#include <glad/glad.h>

using namespace Daedalus;

namespace {

void OpenGLMessageCallback(
	unsigned source,
	unsigned type,
	unsigned id,
	unsigned severity,
	int length,
	const char* message,
	const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         DL_CORE_CRITICAL(message); return;
	case GL_DEBUG_SEVERITY_MEDIUM:       DL_CORE_ERROR(message); return;
	case GL_DEBUG_SEVERITY_LOW:          DL_CORE_WARN(message); return;
	case GL_DEBUG_SEVERITY_NOTIFICATION: DL_CORE_TRACE(message); return;
	}

}

}

void OpenGLRendererAPI::Init()
{
#ifdef DL_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(OpenGLMessageCallback, nullptr);

	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
}

std::unique_ptr<ShaderLibrary> OpenGLRendererAPI::LoadShaderLibrary(const std::string& path, bool recompile)
{ 
	return std::make_unique<OpenGLShaderLibrary>(path, recompile);
}

void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	glViewport(x, y, width, height);
}

void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRendererAPI::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
	vertexArray->Bind();
	uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
	vertexArray->Bind();
	glDrawArrays(GL_LINES, 0, vertexCount);
}

void OpenGLRendererAPI::SetLineWidth(float width)
{
	glLineWidth(width);
}
