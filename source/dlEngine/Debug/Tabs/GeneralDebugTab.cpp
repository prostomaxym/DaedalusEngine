#include "dlpch.h"
#include "GeneralDebugTab.h"
#include "Debug/WorldAxisProjection.h"
#include "Core/Application.h"
#include "ECS/Scene.h"
#include "Renderer/Objects/Camera.h"
#include "Platform/ResourcesMonitor.h"
#include <algorithm>
#include <numeric>
#include <implot.h>

using namespace Daedalus;

namespace
{
	constexpr auto MilliMult = 1000.f;
	const ImVec2 SectionDefaultSize(540, 460);



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

void GeneralDebugTab::Init()
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

}

void GeneralDebugTab::Render(DeltaTime dt)
{
	RenderFPSSection(dt);

	ImGui::SameLine();
	RenderCPUSection(dt);

	ImGui::SameLine();
	RenderRAMSection(dt);

	RenderWorldSection();

	ImGui::SameLine();
	RenderGPUSection(dt);
}

void GeneralDebugTab::RenderFPSSection(DeltaTime dt)
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

		m_frame_times.push_back(ms);

		frametime_1 = CalculateLowPercentile(0.01f);
		frametime_01 = CalculateLowPercentile(0.001f);

		const std::vector<float> last_sec_ft(m_frame_times.begin() + LastSecondIdx, m_frame_times.end());
		avg_ms = Average(last_sec_ft);
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
		ImPlot::PlotLine("Frame Time", m_x_axe_values.data(), ft_data.data(), static_cast<int>(m_frame_times.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });

		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void GeneralDebugTab::RenderCPUSection(DeltaTime dt)
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

		m_cpu_app_load.push_back(app_load);
		m_cpu_total_load.push_back(total_load);

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
		ImPlot::PlotLine("Total CPU", m_x_axe_values.data(), total_data.data(), static_cast<int>(m_cpu_total_load.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });
		ImPlot::PlotLine("App CPU", m_x_axe_values.data(), app_data.data(), static_cast<int>(m_cpu_app_load.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });
		
		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void GeneralDebugTab::RenderRAMSection(DeltaTime dt)
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

		m_ram_app_load.push_back(app_load);
		m_ram_total_load.push_back(total_load);

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
		ImPlot::PlotLine("Total RAM", m_x_axe_values.data(), total_data.data(), static_cast<int>(m_ram_total_load.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });
		ImPlot::PlotLine("App RAM", m_x_axe_values.data(), app_data.data(), static_cast<int>(m_ram_app_load.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });

		ImPlot::EndPlot();
	}

	ImGui::EndChild();
}

void GeneralDebugTab::RenderWorldSection()
{
	ImGui::BeginChild("World", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_Bullet);

	const auto scene = Application::GetInstance()->GetMainScene();
	if (!scene || !scene->IsRunning() || !scene->GetCamera())
	{
		ImGui::TextUnformatted("No active scene camera");
		ImGui::EndChild();
		return;
	}
	const auto& scene_name = scene->GetSceneName();
	ImGui::Text("Scene name: %s", scene_name.c_str());

	const auto camera = scene->GetCamera();
	glm::vec3 pos = camera->GetPosition();
	glm::vec3 dir = camera->GetDirection();

	ImGui::Text("Camera pos (x,y,z):  { %.2f; %.2f; %.2f; }", pos.x, pos.y, pos.z);
	ImGui::Text("Camera dir (x,y,z):  { %.2f; %.2f; %.2f; }", dir.x, dir.y, dir.z);

	std::array<glm::vec2, 3> axis_offsets;
	if (WorldAxisProjection::Calculate(*camera, axis_offsets))
	{
		const auto top_left = ImGui::GetCursorScreenPos();
		ImGui::Dummy(ImVec2(300.f, 300.f));
		const ImVec2 origin(top_left.x + 150.f, top_left.y + 150.f);
		const ImU32 colors[] = { IM_COL32(255, 0, 0, 255), IM_COL32(0, 255, 0, 255), IM_COL32(0, 0, 255, 255) };
		const char* labels[] = { "X", "Y", "Z" };
		auto* draw_list = ImGui::GetWindowDrawList();
		for (size_t axis = 0; axis < axis_offsets.size(); ++axis)
		{
			const auto& offset = axis_offsets[axis];
			const ImVec2 endpoint(origin.x + offset.x * 110.f, origin.y + offset.y * 110.f);
			draw_list->AddLine(origin, endpoint, colors[axis], 3.f);
			draw_list->AddCircleFilled(endpoint, 3.f, colors[axis]);
			// End-on axes share the origin; keep their labels readable in the legend.
			if (glm::length(offset) > 0.05f)
				draw_list->AddText(ImVec2(endpoint.x + 5.f, endpoint.y + 5.f), colors[axis], labels[axis]);
			draw_list->AddText(ImVec2(top_left.x + 20.f + static_cast<float>(axis) * 40.f, top_left.y + 280.f), colors[axis], labels[axis]);
		}
	}
	else
		ImGui::TextUnformatted("World axes outside camera clip range");

	ImGui::EndChild();
}

void GeneralDebugTab::RenderGPUSection(DeltaTime dt)
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

		m_gpu_utillization.push_back(gpu_load);
		m_gpu_memory_utillization.push_back(memory_load);
		m_gpu_memory_used.push_back(memory_used_GB);

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
		ImPlot::PlotLine("GPU Load", m_x_axe_values.data(), gpu.data(), static_cast<int>(m_gpu_utillization.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });
		ImPlot::PlotLine("Memory Load", m_x_axe_values.data(), memory.data(), static_cast<int>(m_gpu_memory_utillization.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });

		ImPlot::EndPlot();
	}

	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("GPU Memory", SectionDefaultSize, ImGuiChildFlags_Borders);
	ImGui::CollapsingHeader("GPU Memory", ImGuiTreeNodeFlags_Bullet);

	ImGui::Text("GPU Clock: %.2f MHz", memory_clock);
	ImGui::Text("GPU Memory Utilization: %.2f %%", avg_gpu_memory_util);
	ImGui::Text("Memory Load: %.2f/%.2f GB", avg_memory, m_total_gpu_ram_GB);

	if (ImPlot::BeginPlot("Memory", ImVec2(plot_width, plot_height), ImPlotFlags_None))
	{
		ImPlot::SetupAxes("Realtime (s)", "Memory (GB)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-NumberOfSecondsTracked, 0.f, plot_min, m_total_gpu_ram_GB, ImPlotCond_Once);

		const auto memory = m_gpu_memory_used.linearize();
		ImPlot::PlotLine("Memory", m_x_axe_values.data(), memory.data(), static_cast<int>(m_gpu_memory_used.size()), { ImPlotProp_Flags, ImPlotLineFlags_Shaded });

		ImPlot::EndPlot();
	}
	ImGui::EndChild();
}

float GeneralDebugTab::CalculateLowPercentile(float percentile) const
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

