#include "dlpch.h"
#include "RenderingDebugTab.h"
#include <imgui.h>
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RendererDebugInfo.h"
#include "Renderer/Resources/Texture.h"

using namespace Daedalus;

namespace
{
	const ImVec2 TextureSectionSize(640, 400);
	const ImVec2 QuadTextureSectionSize(640, 680);
}

void RenderingDebugTab::Render()
{

		auto info = Renderer::GetDebugInfo();
	
		if (ImGui::BeginTabBar("Framebuffers", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Geometry"))
			{
				RenderTexture("Albedo", info->geom.albedo_texture, TextureSectionSize);
				ImGui::SameLine();
				RenderTexture("Ambient", info->geom.ambient_texture, TextureSectionSize);
				ImGui::SameLine();
				RenderTexture("Specular", info->geom.spec_texture, TextureSectionSize);

				RenderTexture("Normal", info->geom.norm_texture, TextureSectionSize);
				ImGui::SameLine();
				RenderTexture("Position", info->geom.pos_texture, TextureSectionSize);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Depth"))
			{
				RenderTexture("Depth", info->greyscale_depth->GetRendererID(), TextureSectionSize);	
				ImGui::SameLine();
				RenderTexture("SSAO", info->greyscale_ssao->GetRendererID(), TextureSectionSize);
				ImGui::SameLine();
				RenderTexture("Shininess", info->greyscale_shininess->GetRendererID(), TextureSectionSize);
				
				RenderTexture("Shadow", info->greyscale_shadow->GetRendererID(), QuadTextureSectionSize, false);

				static int current_item = 0;
				int number_of_maps = info->number_of_shadow_maps;
				auto getter = [](void*, int idx) -> const char*
					{
						static char buf[16];
						snprintf(buf, sizeof(buf), "%d", idx);
						return buf;
					};

				if (ImGui::Combo("Select Shadowmap", &current_item, getter, nullptr, number_of_maps, 4))
				{
					info->current_shadow_map = current_item;
				}	

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
}

void RenderingDebugTab::RenderTexture(std::string_view name, uint32_t texture_id, ImVec2 section_size, bool use_aspect)
{
	ImGui::BeginChild(name.data(), section_size, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader(name.data(), ImGuiTreeNodeFlags_Bullet);

	const auto aspect = Renderer::GetAspectRatio();
	const auto tex_size_x = section_size.x - 10.f;
	const auto tex_size_y = use_aspect ? (tex_size_x / aspect) : tex_size_x;
	const ImVec2 size = ImVec2(tex_size_x, tex_size_y);
	ImGui::Image((ImTextureID)(intptr_t)texture_id, size, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();
}

