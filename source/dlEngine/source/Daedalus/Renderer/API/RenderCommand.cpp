#include "dlpch.h"
#include "RenderCommand.h"

using namespace Daedalus;

std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

void RenderCommand::Init()
{
	s_RendererAPI->Init();
}

void RenderCommand::SetupGraphicSettings()
{
	s_RendererAPI->SetupGraphicSettings();
}

void RenderCommand::SetupShaderSettings()
{
	s_RendererAPI->SetupShaderSettings();
}

std::unique_ptr<ShaderLibrary> RenderCommand::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	return s_RendererAPI->LoadShaderLibrary(path, recompile);
}

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	s_RendererAPI->SetViewport(x, y, width, height);
}

void RenderCommand::SetClearColor(const glm::vec4& color)
{
	s_RendererAPI->SetClearColor(color);
}

void RenderCommand::Clear()
{
	s_RendererAPI->Clear();
}

void RenderCommand::DrawIndexed(const VertexArray* vertex_array, uint32_t index_count)
{
	s_RendererAPI->DrawIndexed(vertex_array, index_count);
}

void RenderCommand::DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count)
{
	s_RendererAPI->DrawUnindexed(vertex_array, vertex_count);
}

void RenderCommand::DrawLines(const VertexArray* vertexArray, uint32_t vertexCount)
{
	s_RendererAPI->DrawLines(vertexArray, vertexCount);
}

void RenderCommand::UnbindTextureSlot(uint32_t slot_num)
{
	s_RendererAPI->UnbindTextureSlot(slot_num);
}

void RenderCommand::SetLineWidth(float width)
{
	s_RendererAPI->SetLineWidth(width);
}