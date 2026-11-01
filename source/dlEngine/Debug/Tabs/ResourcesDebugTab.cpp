#include "dlpch.h"
#include "ResourcesDebugTab.h"
#include <imgui.h>
#include "Renderer/Resources/ResourceManager.h"

using namespace Daedalus;

static void TextWithTooltip(const char* text)
{
	const bool truncated = ImGui::CalcTextSize(text).x > ImGui::GetContentRegionAvail().x;
	ImGui::TextUnformatted(text);
	if (truncated && ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", text);
}

void ResourcesDebugTab::Render()
{


	ImGui::BeginChild("ResourcesScroll", ImVec2(1600.f, 900.f), ImGuiChildFlags_Borders);

	const auto stats = ResourceManager::GetStats();

	if (ImGui::BeginTable("ResourceStats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		const auto row = [](const char* name, uint64_t value)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(name);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%llu", static_cast<unsigned long long>(value));
			};

		row("Shaders", stats.shaders);
		row("Models", stats.models);
		row("Materials", stats.materials);
		row("Meshes", stats.meshes);
		row("Named textures", stats.named_textures);
		row("Unnamed textures", stats.unnamed_textures);
		row("Unnamed cubemaps", stats.unnamed_cubemaps);
		row("Vertices", static_cast<uint64_t>(stats.vertices));
		row("Indices", stats.indices);
		row("Triangles", stats.triangles);

		const auto text_row = [](const char* name, const std::string& value)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(name);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(value.c_str());
			};
		const auto cache_row = [&text_row](const char* name, const CacheCounters& c)
			{
				char buf[96];
				std::snprintf(buf, sizeof(buf), "%llu hits / %llu misses (%.1f%%)", static_cast<unsigned long long>(c.hits), static_cast<unsigned long long>(c.misses), c.HitRatePercent());
				text_row(name, buf);
			};

		text_row("Texture memory", FormatBytes(stats.texture_bytes));
		text_row("Cubemap memory", FormatBytes(stats.cubemap_bytes));
		text_row("Vertex buffers", FormatBytes(stats.vertex_buffer_bytes));
		text_row("Index buffers", FormatBytes(stats.index_buffer_bytes));
		text_row("Total GPU memory", FormatBytes(stats.TotalGpuBytes()));
		cache_row("Model cache", stats.model_cache);
		cache_row("Material cache", stats.material_cache);
		cache_row("Texture cache", stats.texture_cache);

		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Models"))
	{
		if (ImGui::BeginTable("ModelStats", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Meshes");
			ImGui::TableSetupColumn("Materials");
			ImGui::TableSetupColumn("Vertices");
			ImGui::TableSetupColumn("Indices");
			ImGui::TableHeadersRow();

			for (const auto& model : ResourceManager::GetModelStats())
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				TextWithTooltip(model.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%zu", model.meshes);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%zu", model.materials);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%llu", static_cast<unsigned long long>(model.vertices));
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%llu", static_cast<unsigned long long>(model.indices));
			}
			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Textures"))
	{
		if (ImGui::BeginTable("TextureStats", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Size");
			ImGui::TableSetupColumn("GL ID");
			ImGui::TableSetupColumn("Named");
			ImGui::TableSetupColumn("Format");
			ImGui::TableSetupColumn("Memory");
			ImGui::TableSetupColumn("Refs");
			ImGui::TableSetupColumn("Load ms");
			ImGui::TableHeadersRow();

			for (const auto& texture : ResourceManager::GetTextureStats())
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				TextWithTooltip(texture.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%ux%u", texture.width, texture.height);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%u", texture.renderer_id);
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(texture.named ? "yes" : "no");
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(texture.format.c_str());
				ImGui::TableSetColumnIndex(5);
				ImGui::TextUnformatted(FormatBytes(texture.bytes).c_str());
				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%ld", texture.ref_count);
				ImGui::TableSetColumnIndex(7);
				ImGui::Text("%.2f", texture.load_time_ms);
			}
			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Meshes"))
	{
		if (ImGui::BeginTable("MeshStats", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Vertices");
			ImGui::TableSetupColumn("Indices");
			ImGui::TableSetupColumn("Material");
			ImGui::TableSetupColumn("GPU memory");
			ImGui::TableSetupColumn("Radius");
			ImGui::TableHeadersRow();

			for (const auto& mesh : ResourceManager::GetMeshStats())
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				TextWithTooltip(mesh.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%u", mesh.vertices);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%u", mesh.indices);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%u", mesh.material_index);
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(FormatBytes(mesh.vertex_buffer_bytes + mesh.index_buffer_bytes).c_str());
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%.2f", mesh.bounding_radius);
			}
			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Materials"))
	{
		for (const auto& material : ResourceManager::GetMaterialStats())
		{
			if (!ImGui::TreeNode(material.name.c_str()))
				continue;

			ImGui::Text("Ambient: %.2f %.2f %.2f", material.ambient[0], material.ambient[1], material.ambient[2]);
			ImGui::Text("Diffuse: %.2f %.2f %.2f", material.diffuse[0], material.diffuse[1], material.diffuse[2]);
			ImGui::Text("Specular: %.2f %.2f %.2f", material.specular[0], material.specular[1], material.specular[2]);
			ImGui::Text("Shininess: %.2f", material.shininess);
			ImGui::Text("References: %ld", material.ref_count);
			for (const auto& tex : material.textures)
				ImGui::Text("%s map: GL %u (%ux%u)", tex.slot, tex.renderer_id, tex.width, tex.height);
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Cubemaps"))
	{
		for (const auto& cubemap : ResourceManager::GetCubemapStats())
		{
			ImGui::Text("GL %u  %ux%u  %s  %s  refs %ld  load %.2f ms", cubemap.renderer_id, cubemap.face_width, cubemap.face_height,
				cubemap.format.c_str(), FormatBytes(cubemap.bytes).c_str(), cubemap.ref_count, cubemap.load_time_ms);
		}
	}

	if (ImGui::CollapsingHeader("Shaders"))
	{
		const auto draw_variables = [](const char* table_id, const std::vector<ShaderVariableInfo>& vars)
			{
				if (ImGui::BeginTable(table_id, 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Type");
					ImGui::TableSetupColumn("Size");
					ImGui::TableSetupColumn("Location");
					ImGui::TableHeadersRow();

					for (const auto& var : vars)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						TextWithTooltip(var.name.c_str());
						ImGui::TableSetColumnIndex(1);
						ImGui::TextUnformatted(var.type.c_str());
						ImGui::TableSetColumnIndex(2);
						ImGui::Text("%d", var.size);
						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%d", var.location);
					}
					ImGui::EndTable();
				}
			};

		for (const auto& shader : ResourceManager::GetShaderStats())
		{
			const auto& name = shader.name;
			const auto& info = shader.info;
			if (!ImGui::TreeNode(name.c_str()))
				continue;

			ImGui::Text("Origin: %s", shader.load.origin.c_str());
			ImGui::Text("Source: %s", shader.load.source_path.c_str());
			ImGui::Text("Load time: %.2f ms", shader.load.load_time_ms);
			if (shader.load.has_last_modified)
			{
				const auto t = std::chrono::system_clock::to_time_t(shader.load.last_modified);
				char buf[32] = {};
				std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
				ImGui::Text("Source modified: %s", buf);
			}
			if (!info.info_log.empty())
				ImGui::TextWrapped("Info log: %s", info.info_log.c_str());
			ImGui::Text("GL program ID: %u", info.renderer_id);
			ImGui::Text("Linked: %s", info.linked ? "yes" : "no");
			ImGui::Text("Binary size: %d bytes", info.binary_length);
			ImGui::Text("Type: %s", info.is_compute ? "compute" : "graphics");
			if (info.is_compute)
				ImGui::Text("Work group size: %d x %d x %d", info.compute_work_group_size[0], info.compute_work_group_size[1], info.compute_work_group_size[2]);
			ImGui::Text("Uniform blocks: %d", info.uniform_block_count);

			if (ImGui::TreeNode("Uniforms", "Uniforms (%zu)", info.uniforms.size()))
			{
				draw_variables("Uniforms", info.uniforms);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Attributes", "Attributes (%zu)", info.attributes.size()))
			{
				draw_variables("Attributes", info.attributes);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}

	ImGui::EndChild();
}

