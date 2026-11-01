#include "dlpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/Primitives/VertexArray.h"
#include "Renderer/Primitives/IndexBuffer.h"
#include "Renderer/Primitives/VertexBuffer.h"
#include "Renderer/Primitives/BufferLayout.h"
#include "Renderer/Resources/Shader.h"
#include "Renderer/Resources/Texture.h"
#include <glm/vec4.hpp>

#include "Renderer/Resources/ResourceManager.h"

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
		case GL_DEBUG_SEVERITY_HIGH:         Log::Write(Log::Levels::Critical, Log::Categories::RendererAPI, "{}", message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       Log::Write(Log::Levels::Error, Log::Categories::RendererAPI, "{}", message); return;
		case GL_DEBUG_SEVERITY_LOW:         Log::Write(Log::Levels::Warn, Log::Categories::RendererAPI, "{}", message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: /*Log::Write(Log::Levels::Trace, Log::Categories::Renderer, message)*/; return;
		}
	}

}

void OpenGLRendererAPI::Init()
{
	ConfigLogging();
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Renderer inited;");
}

void OpenGLRendererAPI::SetupGraphicSettings()
{
	if (Daedalus::GraphicsConfig::IsMultisampleEnabled())
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
	
	//if (Daedalus::GraphicsConfig::IsMultisampleEnabled())
	//{
	//	glEnable(GL_BLEND);
	//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//}
	//else
	//{
	//	glDisable(GL_BLEND);
	//}
	glDisable(GL_BLEND);
	
	if (Daedalus::GraphicsConfig::IsMultisampleEnabled())
		glEnable(GL_LINE_SMOOTH);
	else
		glDisable(GL_LINE_SMOOTH);
		
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	glViewport(x, y, width, height);
}

void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRendererAPI::Clear(uint32_t clear_mode)
{
	glClear(clear_mode);
}

void OpenGLRendererAPI::DrawIndexed(const VertexArray* vertex_array, uint32_t index_count)
{
	vertex_array->Bind();
	uint32_t count = index_count ? index_count : vertex_array->GetIndexBuffer()->GetCount();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	vertex_array->Unbind();
}

void OpenGLRendererAPI::DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count)
{
	vertex_array->Bind();
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
	vertex_array->Unbind();
}

void OpenGLRendererAPI::DrawLines(const VertexArray* vertex_array, uint32_t vertexCount)
{
	vertex_array->Bind();
	glDrawArrays(GL_LINES, 0, vertexCount);
	vertex_array->Unbind();
}

void OpenGLRendererAPI::SetLineWidth(float width)
{
	glLineWidth(width);
}

void OpenGLRendererAPI::DrawDebugLines(const VertexArray* vertex_array, VertexBuffer* vertex_buffer, const Shader* shader,
	const void* vertices, uint32_t vertex_count, uint32_t vertex_size, bool depth_test)
{
	GLint program, vao, buffer, framebuffer, depth_func;
	GLboolean depth_mask, color_mask[4];
	GLfloat line_width;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
	glGetIntegerv(GL_DEPTH_FUNC, &depth_func);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask);
	glGetBooleani_v(GL_COLOR_WRITEMASK, 0, color_mask);
	glGetFloatv(GL_LINE_WIDTH, &line_width);
	const auto blend_enabled = glIsEnabledi(GL_BLEND, 0);
	glDisablei(GL_BLEND, 0);
	const std::array<GLenum, 5> capabilities = { GL_DEPTH_TEST, GL_LINE_SMOOTH, GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_COLOR_LOGIC_OP };
	std::array<GLboolean, 5> enabled;
	for (size_t i = 0; i < capabilities.size(); ++i)
	{
		enabled[i] = glIsEnabled(capabilities[i]);
		glDisable(capabilities[i]);
	}
	if (depth_test) glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glLineWidth(1.f);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	vertex_buffer->SetData(vertices, vertex_count * vertex_size);
	shader->Bind();
	DrawLines(vertex_array, vertex_count);
	glUseProgram(program);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	glDepthFunc(depth_func);
	glDepthMask(depth_mask);
	glColorMaski(0, color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
	glLineWidth(line_width);
	if (blend_enabled) glEnablei(GL_BLEND, 0); else glDisablei(GL_BLEND, 0);
	for (size_t i = 0; i < capabilities.size(); ++i)
		if (enabled[i]) glEnable(capabilities[i]); else glDisable(capabilities[i]);
}

void OpenGLRendererAPI::UnbindTextureSlot(uint32_t slot_number)
{
	// Using blank texture to get rid of warning when unbinding texture at all

	static std::shared_ptr<Texture> blank_texture{nullptr};

	if (!blank_texture)
	{
		unsigned char black_pixel[] = { 255, 255, 255, 255 };
		blank_texture = ResourceManager::LoadTexture2D(black_pixel, 1, 1, 4);
		blank_texture->Bind(slot_number);
	}
	else
	{
		blank_texture->Bind(slot_number);
	}
}

int OpenGLRendererAPI::GetMaxTextureSize()
{
	int max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

	return max_texture_size;;
}

void OpenGLRendererAPI::ConfigLogging()
{
	if (IsDevBuild())
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_WINDOW_SYSTEM, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_OTHER, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	}
}