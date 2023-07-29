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
	case GL_DEBUG_SEVERITY_HIGH:         Log::Write(Log::Levels::Critical, Log::Categories::Renderer, message); return;
	case GL_DEBUG_SEVERITY_MEDIUM:       Log::Write(Log::Levels::Error, Log::Categories::Renderer, message); return;
	case GL_DEBUG_SEVERITY_LOW:         /*Log::Write(Log::Levels::Warn, Log::Categories::Renderer, message);*/ return;
	case GL_DEBUG_SEVERITY_NOTIFICATION: /*Log::Write(Log::Levels::Trace, Log::Categories::Renderer, message);*/ return;
	}

}

}

void OpenGLRendererAPI::Init()
{
#ifdef DL_DEBUG_BUILD
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(OpenGLMessageCallback, nullptr);

	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageControl(GL_DEBUG_SOURCE_WINDOW_SYSTEM, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	//glDebugMessageControl(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	//glDebugMessageControl(GL_DEBUG_SOURCE_OTHER, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Renderer inited;");
}

std::unique_ptr<ShaderLibrary> OpenGLRendererAPI::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
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

void OpenGLRendererAPI::DrawIndexed(const VertexArray* vertex_array, uint32_t index_count)
{
	vertex_array->Bind();
	uint32_t count = index_count ? index_count : vertex_array->GetIndexBuffer()->GetCount();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count)
{
	vertex_array->Bind();
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}


void OpenGLRendererAPI::DrawLines(const VertexArray* vertexArray, uint32_t vertexCount)
{
	vertexArray->Bind();
	glDrawArrays(GL_LINES, 0, vertexCount);
}

void OpenGLRendererAPI::SetLineWidth(float width)
{
	glLineWidth(width);
}
