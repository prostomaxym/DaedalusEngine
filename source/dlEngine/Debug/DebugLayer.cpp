#include "dlpch.h"

#include "DebugLayer.h"
#include "Core/Application.h"

#include "Renderer/API/Renderer.h"
#include "Platform/ResourcesMonitor.h"

#include <implot.h>
#include <implot3d.h>

using namespace Daedalus;

namespace
{
	constexpr auto MilliMult = 1000.f;
	const ImVec2 SectionDefaultSize(540, 460);
	const ImVec2 TextureSectionSize(640, 400);
	const ImVec2 QuadTextureSectionSize(640, 680);

	float Average(const std::vector<float>& v)
	{
		if (v.empty())
			return 0.0;

		const auto it = std::find_if(v.begin(), v.end(),
			[](const auto val) {
				return val <= 0.f;
			});

		float sum = std::accumulate(v.begin(), it, 0.0);

		if (v.size() > 0)
			return sum / v.size();
		else
			return 0.f;
	}

	glm::vec3 RotateAxisFromGLtoPlot(const glm::vec3& vec)
	{
		auto copy = vec;
		copy.y = -vec.z;
		copy.z = vec.y;

		return copy;
	}
}

void DebugLayer::Update(DeltaTime dt)
{
	ImGui::Begin("Debug Overlay", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::BeginTabBar("Debug Overlay", ImGuiTabBarFlags_None))
	{
		RenderGeneralPage(dt);
		RenderRenderingPage();

		if (ImGui::BeginTabItem("Tab 3"))
		{
			ImGui::Text("This is Tab 3");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}

void DebugLayer::InitWindow()
{
	m_frame_times.resize(GraphSize, 0.f);
	m_cpu_app_load.resize(GraphSize, 0.f);
	m_cpu_total_load.resize(GraphSize, 0.f);
	m_ram_app_load.resize(GraphSize, 0.f);
	m_ram_total_load.resize(GraphSize, 0.f);
	m_x_axe_values.resize(GraphSize, 0.f);
	m_gpu_utillization.resize(GraphSize, 0.f);
	m_gpu_memory_utillization.resize(GraphSize, 0.f);
	m_gpu_memory_used.resize(GraphSize, 0.f);

	for (auto i = 0; i < m_x_axe_values.size(); i++)
	{
		m_x_axe_values[i] = -NumberOfSecondsTracked + NumberOfSecondsTracked / static_cast<float>(GraphSize) * static_cast<float>(i);
	}

	const auto& info = ResourcesMonitor::GetGPUInfo();

	m_total_ram_GB = ResourcesMonitor::GetSystemRAMInGB();
	m_total_gpu_ram_GB = info.memory_total_GB;
	m_processor_name = ResourcesMonitor::GetProcessorName();
	m_gpu_name = !info.name.empty() ? info.name : "Unsupported";

	// All sizes are suitable for QHD, scale for big for FHD to fit everything on screen
	// Maybe will implement scalability of debug layer later, right now just scale
	const auto [w,h] = Renderer::GetResolution();
	if (w <= 1920)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(1.0 / 1.33f);
	}
}

void DebugLayer::RenderGeneralPage(DeltaTime dt)
{
	if (ImGui::BeginTabItem("General"))
	{
		RenderFPSSection(dt);

		ImGui::SameLine();
		RenderCPUSection(dt);

		ImGui::SameLine();
		RenderRAMSection(dt);

		RenderWorldSection();

		ImGui::SameLine();
		RenderGPUSection(dt);

		ImGui::EndTabItem();
	}
}

void DebugLayer::RenderFPSSection(DeltaTime dt)
{
	ImGui::BeginChild("FPS", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("FPS", ImGuiTreeNodeFlags_Bullet);

	static float time_accum = 0.f;
	static int frame_count = 0;
	static float fps = 0.f;
	static float ms = 0.f;
	static float frametime_1 = 0.f;
	static float frametime_01 = 0.f;
	static float avg_fps = 0.f;
	static float avg_ms = 0.f;
	static int frame_index = 0;

	time_accum += dt.GetMilliseconds();
	frame_count++;

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		fps = MilliMult * static_cast<float>(frame_count) / time_accum;
		ms = MilliMult / fps;
		time_accum = 0.0f;
		frame_count = 0;

		m_frame_times.push(ms);

		frametime_1 = CalculateLowPercentile(0.01f);
		frametime_01 = CalculateLowPercentile(0.001f);

		const std::vector<float> last_sec_ft(m_frame_times.begin() + LastSecondIdx, m_frame_times.end());
		const auto avg_ms = Average(last_sec_ft);
		if (avg_ms > 0.f)
		{
			avg_fps = MilliMult / avg_ms;
		}
	}

	ImGui::Text("Average FPS: %.2f", avg_fps);
	ImGui::Text("1%% FPS: %.2f", MilliMult / frametime_1);
	ImGui::Text("0.1%% FPS: %.2f", MilliMult / frametime_01);
	ImGui::Text("Frame Time: %.2f ms", avg_ms);

	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	constexpr float plot_max = 50.0f;
	constexpr float plot_min = 0.0f;

	if (ImPlot::BeginPlot("Frame Time", ImVec2(plot_width, plot_height)))
	{
		ImPlot::SetupAxes("Realtime (s)", "Frametime (ms)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, plot_max, ImPlotCond_Once);

		const auto ft_data = m_frame_times.linearize();
		ImPlot::PlotLine("Frame Time", m_x_axe_values.data(), ft_data.data(), static_cast<int>(m_frame_times.size()), ImPlotLineFlags_Shaded);

		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void DebugLayer::RenderCPUSection(DeltaTime dt)
{
	ImGui::BeginChild("CPU", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("CPU", ImGuiTreeNodeFlags_Bullet);

	static float time_accum = 0.f;
	static float app_load = 0.f;
	static float total_load = 0.f;
	static float avg_app_load = 0.f;
	static float avg_total_load = 0.f;

	time_accum += dt.GetMilliseconds();

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		app_load = ResourcesMonitor::GetAppCPUUsage();
		total_load = ResourcesMonitor::GetTotalCPUUsage();
		time_accum = 0.0f;;

		m_cpu_app_load.push(app_load);
		m_cpu_total_load.push(total_load);

		const std::vector<float> last_sec_app(m_cpu_app_load.begin() + LastSecondIdx, m_cpu_app_load.end());
		const std::vector<float> last_sec_total(m_cpu_total_load.begin() + LastSecondIdx, m_cpu_total_load.end());
		avg_app_load = Average(last_sec_app);
		avg_total_load = Average(last_sec_total);
	}

	ImGui::Text("Processor: %s", m_processor_name.c_str());
	ImGui::Text("Application CPU load: %.2f %%", avg_app_load);
	ImGui::Text("Total CPU load: %.2f %%", avg_total_load);

	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	constexpr float plot_max = 100.f;
	constexpr float plot_min = 0.0f;

	if (ImPlot::BeginPlot("CPU Load", ImVec2(plot_width, plot_height), ImPlotFlags_None))
	{
		ImPlot::SetupAxes("Realtime (s)", "CPU (%)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, plot_max, ImPlotCond_Once);

		const auto app_data = m_cpu_app_load.linearize();
		const auto total_data = m_cpu_total_load.linearize();
		ImPlot::PlotLine("Total CPU", m_x_axe_values.data(), total_data.data(), static_cast<int>(m_cpu_total_load.size()), ImPlotLineFlags_Shaded);
		ImPlot::PlotLine("App CPU", m_x_axe_values.data(), app_data.data(), static_cast<int>(m_cpu_app_load.size()), ImPlotLineFlags_Shaded);
		
		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void DebugLayer::RenderRAMSection(DeltaTime dt)
{
	ImGui::BeginChild("RAM", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("RAM", ImGuiTreeNodeFlags_Bullet);

	static float time_accum = 0.f;
	static float app_load = 0.f;
	static float total_load = 0.f;
	static float avg_app_load = 0.f;
	static float avg_total_load = 0.f;\

	time_accum += dt.GetMilliseconds();

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		app_load = ResourcesMonitor::GetAppRAMUsageInGB();
		total_load = ResourcesMonitor::GetTotalRAMUsageInGB();
		time_accum = 0.0f;

		m_ram_app_load.push(app_load);
		m_ram_total_load.push(total_load);

		const std::vector<float> last_sec_app(m_ram_app_load.begin() + LastSecondIdx, m_ram_app_load.end());
		const std::vector<float> last_sec_total(m_ram_total_load.begin() + LastSecondIdx, m_ram_total_load.end());
		avg_app_load = Average(last_sec_app);
		avg_total_load = Average(last_sec_total);
	}

	ImGui::Text("Application RAM load: %.3f GB", avg_app_load);
	ImGui::Text("Total RAM load: %.3f GB", avg_total_load);

	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	float plot_max = m_total_ram_GB;
	constexpr float plot_min = 0.0f;

	if (ImPlot::BeginPlot("RAM Load", ImVec2(plot_width, plot_height), ImPlotFlags_None))
	{
		ImPlot::SetupAxes("Realtime (s)", "RAM (GB)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, plot_max, ImPlotCond_Once);

		const auto app_data = m_ram_app_load.linearize();
		const auto total_data = m_ram_total_load.linearize();
		ImPlot::PlotLine("Total RAM", m_x_axe_values.data(), total_data.data(), static_cast<int>(m_ram_total_load.size()), ImPlotLineFlags_Shaded);
		ImPlot::PlotLine("App RAM", m_x_axe_values.data(), app_data.data(), static_cast<int>(m_ram_app_load.size()), ImPlotLineFlags_Shaded);

		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void DebugLayer::RenderWorldSection()
{
	ImGui::BeginChild("World", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_Bullet);

	const auto scene = Application::GetInstance()->GetMainScene();
	const auto& scene_name = scene->GetSceneName();
	ImGui::Text("Scene name: %s", scene_name.c_str());

	const auto camera = scene->GetCamera();
	glm::vec3 pos = camera->GetPosition();
	glm::vec3 dir = camera->GetDirection();

	ImGui::Text("Camera pos (x,y,z):  { %.2f; %.2f; %.2f; }", pos.x, pos.y, pos.z);
	ImGui::Text("Camera dir (x,y,z):  { %.2f; %.2f; %.2f; }", dir.x, dir.y, dir.z);

	ImPlot3DFlags flags = ImPlot3DFlags_CanvasOnly |
		ImPlot3DFlags_NoTitle |
		ImPlot3DFlags_NoClip;

	if (ImPlot3D::BeginPlot("##WorldAxes", ImVec2(300, 300), flags))
	{
		glm::vec3 cam_dir = glm::normalize(dir);
		glm::vec3 cam_up = glm::normalize(camera->GetUp());
		glm::vec3 cam_right = glm::normalize(glm::cross(cam_dir, cam_up));

		glm::mat3 cam_rotation = glm::mat3(
			cam_right,
			cam_up,
			cam_dir
		);

		glm::vec3 x_axis = cam_rotation * glm::vec3(1.f, 0, 0);
		glm::vec3 y_axis = cam_rotation * glm::vec3(0, 1.f, 0);
		glm::vec3 z_axis = cam_rotation * glm::vec3(0, 0, 1.f);

		pos = RotateAxisFromGLtoPlot(pos);
		x_axis = RotateAxisFromGLtoPlot(x_axis);
		y_axis = RotateAxisFromGLtoPlot(y_axis);
		z_axis = RotateAxisFromGLtoPlot(z_axis);

		ImPlot3D::SetupAxesLimits(pos.x - 1.f, pos.x + 1.f, pos.y - 1.f, pos.y + 1.f, pos.z - 1.f, pos.z + 1.f, ImPlot3DCond_Always);
		ImPlot3D::SetupAxes("X", "Y", "Z"
			, ImPlot3DAxisFlags_NoDecorations
			, ImPlot3DAxisFlags_NoDecorations | ImPlot3DAxisFlags_Invert
			, ImPlot3DAxisFlags_NoDecorations);

		ImPlot3D::SetupBoxRotation(0.f, 0.f, false, ImPlot3DCond_Always);
		ImPlot3D::SetupBoxScale(1.5f, 1.5f, 1.5f);

		// X axis
		{
			float x[2] = { pos.x, pos.x + x_axis.x };
			float y[2] = { pos.y, pos.y + x_axis.y };
			float z[2] = { pos.z, pos.z + x_axis.z };
			ImPlot3D::PushStyleColor(ImPlot3DCol_Line, ImVec4(1, 0, 0, 1));
			ImPlot3D::PlotLine("X", x, y, z, 2);
			ImPlot3D::PopStyleColor();
		}

		// Y axis
		{
			float x[2] = { pos.x, pos.x + y_axis.x };
			float y[2] = { pos.y, pos.y + y_axis.y };
			float z[2] = { pos.z, pos.z + y_axis.z };
			ImPlot3D::PushStyleColor(ImPlot3DCol_Line, ImVec4(0, 1, 0, 1));
			ImPlot3D::PlotLine("Y", x, y, z, 2);
			ImPlot3D::PopStyleColor();
		}

		// Z axis
		{
			float x[2] = { pos.x, pos.x + z_axis.x };
			float y[2] = { pos.y, pos.y + z_axis.y };
			float z[2] = { pos.z, pos.z + z_axis.z };
			ImPlot3D::PushStyleColor(ImPlot3DCol_Line, ImVec4(0, 0.5f, 1, 1));
			ImPlot3D::PlotLine("Z", x, y, z, 2);
			ImPlot3D::PopStyleColor();
		}

		ImPlot3D::EndPlot();
	}

	ImGui::EndChild();
}

void DebugLayer::RenderGPUSection(DeltaTime dt)
{
	ImGui::BeginChild("GPU", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_Bullet);

	static float time_accum = 0.f;
	static float gpu_load = 0.f;
	static float temp = 0.f;
	static float power = 0.f;
	static float clock = 0.f;

	static float memory_load = 0.f;
	static float memory_clock = 0.f;
	static float gpu_clock = 0.f;
	static float memory_used_GB = 0.f;

	static float avg_gpu_util = 0.f;
	static float avg_gpu_memory_util = 0.f;
	static float avg_memory = 0.f;

	time_accum += dt.GetMilliseconds();

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		const auto& info = ResourcesMonitor::GetGPUInfo();

		time_accum = 0.f;
		gpu_load = static_cast<float>(info.gpu_utilization_percent);
		temp = static_cast<float>(info.temperature_celsius);
		power = static_cast<float>(info.power_usage_watt);
		gpu_clock = static_cast<float>(info.graphics_clock_mhz);

		memory_load = static_cast<float>(info.memory_utilization_percent);
		memory_clock = static_cast<float>(info.memory_clock_mhz);
		memory_used_GB = static_cast<float>(info.memory_used_GB);

		m_gpu_utillization.push(gpu_load);
		m_gpu_memory_utillization.push(memory_load);
		m_gpu_memory_used.push(memory_used_GB);

		const std::vector<float> last_sec_gpu(m_gpu_utillization.begin() + LastSecondIdx, m_gpu_utillization.end());
		const std::vector<float> last_sec_gpumem(m_gpu_memory_utillization.begin() + LastSecondIdx, m_gpu_memory_utillization.end());
		const std::vector<float> last_sec_mem(m_gpu_memory_used.begin() + LastSecondIdx, m_gpu_memory_used.end());
		avg_gpu_util = Average(last_sec_gpu);
		avg_gpu_memory_util = Average(last_sec_gpumem);
		avg_memory = Average(last_sec_mem);
	}

	ImGui::Text("GPU Processor: %s", m_gpu_name.c_str());
	ImGui::Text("GPU Load: %.2f %%", avg_gpu_util);
	ImGui::Text("GPU Clock: %.1f MHz", memory_clock);
	ImGui::Text("Temperature: %.1f C", temp);
	ImGui::Text("Power: %.2f W", power);
	
	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	constexpr float plot_max = 100.f;
	constexpr float plot_min = 0.0f;

	if (ImPlot::BeginPlot("GPU Utilization", ImVec2(plot_width, plot_height), ImPlotFlags_None))
	{
		ImPlot::SetupAxes("Realtime (s)", "GPU (%)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, plot_max, ImPlotCond_Once);

		const auto gpu = m_gpu_utillization.linearize();
		const auto memory = m_gpu_memory_utillization.linearize();
		ImPlot::PlotLine("GPU", m_x_axe_values.data(), gpu.data(), static_cast<int>(m_gpu_utillization.size()), ImPlotLineFlags_Shaded);
		ImPlot::PlotLine("Memory", m_x_axe_values.data(), memory.data(), static_cast<int>(m_gpu_memory_utillization.size()), ImPlotLineFlags_Shaded);

		ImPlot::EndPlot();
	}

	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("GPU Memory", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("GPU Memory", ImGuiTreeNodeFlags_Bullet);

	ImGui::Text("GPU Clock: %.2f MHz", memory_clock);
	ImGui::Text("GPU Memory Utilization: %.2f %%", avg_gpu_memory_util);
	ImGui::Text("Memory Load: %.2f/%.2f GB", avg_memory, m_total_gpu_ram_GB);

	if (ImPlot::BeginPlot("GPU Memory", ImVec2(plot_width, plot_height), ImPlotFlags_None))
	{
		ImPlot::SetupAxes("Realtime (s)", "Memory (GB)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, m_total_gpu_ram_GB, ImPlotCond_Once);

		const auto memory = m_gpu_memory_used.linearize();
		ImPlot::PlotLine("Memory", m_x_axe_values.data(), memory.data(), static_cast<int>(m_gpu_memory_used.size()), ImPlotLineFlags_Shaded);

		ImPlot::EndPlot();
	}
	ImGui::EndChild();
}

void DebugLayer::RenderRenderingPage()
{
	if (ImGui::BeginTabItem("Rendering"))
	{
		const auto info = Renderer::GetDebugInfo();
	
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

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::EndTabItem();
	}
}

void DebugLayer::RenderTexture(std::string_view name, uint32_t texture_id, ImVec2 section_size, bool use_aspect)
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

float DebugLayer::CalculateLowPercentile(float percentile) const
{
	if (m_frame_times.empty())
		return 1.0f;

	const auto data = m_frame_times.linearize();
	std::vector<float> sorted(data.begin() + LastSecondIdx, data.end());
	std::sort(sorted.begin(), sorted.end());

	size_t count = static_cast<size_t>(percentile * sorted.size());
	count = std::max(count, size_t(1));

	float sum = 0.0f;
	for (size_t i = 1; i <= count; ++i)
		sum += sorted[sorted.size() - i];

	return sum > 0.f ? sum / static_cast<float>(count) : 1.f;
}