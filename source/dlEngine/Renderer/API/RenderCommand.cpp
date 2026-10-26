#include "dlpch.h"
#include "RenderCommand.h"

using namespace Daedalus;

RendererAPI* RenderCommand::s_renderer_api = nullptr;
std::thread::id RenderCommand::s_owner_thread;

void RenderCommand::Bind(RendererAPI* api)
{
	s_renderer_api = api;
	s_owner_thread = std::this_thread::get_id();
}

RendererAPI& RenderCommand::GetAPI()
{
	DL_ASSERT(s_renderer_api && s_owner_thread == std::this_thread::get_id(), Log::Categories::Renderer, "RenderCommand requires an active renderer on the graphics thread");
	if (!s_renderer_api || s_owner_thread != std::this_thread::get_id()) std::abort();
	return *s_renderer_api;
}

void RenderCommand::Init()
{
	GetAPI().Init();
}

void RenderCommand::SetupGraphicSettings()
{
	GetAPI().SetupGraphicSettings();
}

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	GetAPI().SetViewport(x, y, width, height);
}

void RenderCommand::SetClearColor(const glm::vec4& color)
{
	GetAPI().SetClearColor(color);
}

void RenderCommand::Clear(uint32_t clear_mode)
{
	GetAPI().Clear(clear_mode);
}

void RenderCommand::DrawIndexed(const VertexArray* vertex_array, uint32_t index_count)
{
	GetAPI().DrawIndexed(vertex_array, index_count);
}

void RenderCommand::DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count)
{
	GetAPI().DrawUnindexed(vertex_array, vertex_count);
}

void RenderCommand::DrawLines(const VertexArray* vertexArray, uint32_t vertexCount)
{
	GetAPI().DrawLines(vertexArray, vertexCount);
}

void RenderCommand::UnbindTextureSlot(uint32_t slot_num)
{
	GetAPI().UnbindTextureSlot(slot_num);
}

int RenderCommand::GetMaxTextureSize()
{
	return GetAPI().GetMaxTextureSize();
}

void RenderCommand::SetLineWidth(float width)
{
	GetAPI().SetLineWidth(width);
}