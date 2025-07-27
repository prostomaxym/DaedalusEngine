#include "dlpch.h"

#include "DebugLayer.h"
#include "Core/Application.h"

#include "Platform/ResourcesMonitor.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>
#include <implot.h>

using namespace Daedalus;

namespace
{
	constexpr auto MilliMult = 1000.f;
	const ImVec2 SectionDefaultSize(540, 440);

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
}

void DebugLayer::Update(DeltaTime dt)
{
	ImGui::Begin("Debug Overlay", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::BeginTabBar("Debug Overlay", ImGuiTabBarFlags_None))
	{
		RenderGeneralPage(dt);

		if (ImGui::BeginTabItem("Tab 2"))
		{
			ImGui::Text("This is Tab 2");
			ImGui::EndTabItem();
		}

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

	for (auto i = 0; i < m_x_axe_values.size(); i++)
	{
		m_x_axe_values[i] = -NumberOfSecondsTracked + NumberOfSecondsTracked / static_cast<float>(GraphSize) * static_cast<float>(i);
	}

	m_total_ram = ResourcesMonitor::GetSystemRAMInGB();
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

	if (ImPlot::BeginPlot("Frame Time", ImVec2(plot_width, plot_height), ImPlotFlags_NoLegend))
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
	static int frame_count = 0;
	static float app_load = 0.f;
	static float total_load = 0.f;
	static float accum_app_load = 0.f;
	static float accum_total_load = 0.f;
	static float avg_app_load = 0.f;
	static float avg_total_load = 0.f;

	app_load += ResourcesMonitor::GetAppCPUUsage();
	total_load += ResourcesMonitor::GetTotalCPUUsage();
	time_accum += dt.GetMilliseconds();
	frame_count++;

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		accum_app_load = app_load / frame_count;
		accum_total_load = total_load / frame_count;
		time_accum = 0.0f;
		frame_count = 0;
		app_load = 0.f;
		total_load = 0.f;

		m_cpu_app_load.push(accum_app_load);
		m_cpu_total_load.push(accum_total_load);

		const std::vector<float> last_sec_app(m_cpu_app_load.begin() + LastSecondIdx, m_cpu_app_load.end());
		const std::vector<float> last_sec_total(m_cpu_total_load.begin() + LastSecondIdx, m_cpu_total_load.end());
		avg_app_load = Average(last_sec_app);
		avg_total_load = Average(last_sec_total);
	}

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
	static int frame_count = 0;
	static float app_load = 0.f;
	static float total_load = 0.f;
	static float accum_app_load = 0.f;
	static float accum_total_load = 0.f;
	static float avg_app_load = 0.f;
	static float avg_total_load = 0.f;\

	app_load += ResourcesMonitor::GetAppRAMUsageInGB();
	total_load += ResourcesMonitor::GetTotalRAMUsageInGB();
	time_accum += dt.GetMilliseconds();
	frame_count++;

	if (time_accum >= NumberOfSecondsTracked / GraphSize * MilliMult)
	{
		accum_app_load = app_load / frame_count;
		accum_total_load = total_load / frame_count;
		time_accum = 0.0f;
		frame_count = 0;
		app_load = 0.f;
		total_load = 0.f;

		m_ram_app_load.push(accum_app_load);
		m_ram_total_load.push(accum_total_load);

		const std::vector<float> last_sec_app(m_ram_app_load.begin() + LastSecondIdx, m_ram_app_load.end());
		const std::vector<float> last_sec_total(m_ram_total_load.begin() + LastSecondIdx, m_ram_total_load.end());
		avg_app_load = Average(last_sec_app);
		avg_total_load = Average(last_sec_total);
	}

	ImGui::Text("Application RAM load: %.3f GB", avg_app_load);
	ImGui::Text("Total RAM load: %.3f GB", avg_total_load);

	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	float plot_max = m_total_ram;
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
	glm::vec3 up = camera->GetUp();
	glm::vec3 right = glm::cross(dir, up);

	ImGui::Text("Camera pos (x,y,z):  { %.2f; %.2f; %.2f; }", pos.x, pos.y, pos.z);
	ImGui::Text("Camera dir (x,y,z):  { %.2f; %.2f; %.2f; }", dir.x, dir.y, dir.z);

	if (ImPlot::BeginPlot("Camera Vectors", "X", "Y", ImVec2(300, 300)))
	{
		double origin[2] = { 0.f, 0.f };

		double dir_vec[2] = { dir.x, dir.y };
		double up_vec[2] = { up.x, up.y };
		double right_vec[2] = { right.x, right.y };

		ImPlot::PlotLine("Forward", origin, dir_vec, 2);
		ImPlot::PlotLine("Up", origin, up_vec, 2);
		ImPlot::PlotLine("Right", origin, right_vec, 2);

		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void DebugLayer::RenderGPUSection(DeltaTime dt)
{
	ImGui::BeginChild("GPU", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_Bullet);

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