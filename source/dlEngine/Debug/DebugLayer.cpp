#include "dlpch.h"

#include "DebugLayer.h"

#include "Threads/DaedalusThreads.h"
#include "Platform/ResourcesMonitor.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>
#include <implot.h>

using namespace Daedalus;

namespace
{
	constexpr auto NanoMult = 1000000000.f;
	constexpr auto MilliMult = 1000.f;
}
void DebugLayer::Update(DeltaTime dt)
{
	RenderFPSSection(dt);

	const auto cpu = ResourcesMonitor::GetAppCPUUsage();
	const auto ram = ResourcesMonitor::GetAppRAMUsageInMB();
	//// Renderer info
	//const auto& renderer = Renderer::Get();
	//ImGui::SeparatorText("Renderer");
	//ImGui::Text("Vendor:   %s", renderer.GetVendor().c_str());
	//ImGui::Text("Renderer: %s", renderer.GetRenderer().c_str());
	//ImGui::Text("Version:  %s", renderer.GetVersion().c_str());

	// Optional: backend-specific stats
	//const auto& stats = renderer.GetStats();
	//ImGui::SeparatorText("Pipeline Stats");
	//ImGui::Text("Draw Calls: %d", stats.draw_calls);
	//ImGui::Text("Vertices:   %d", stats.vertex_count);
	//ImGui::Text("Indices:    %d", stats.index_count);

	//// Optional: memory tracking
	//const auto& mem = MemoryTracker::Get();
	//ImGui::SeparatorText("Memory");
	//ImGui::Text("Allocated: %.2f MB", mem.GetAllocated() / (1024.0f * 1024.0f));
	//ImGui::Text("Used:      %.2f MB", mem.GetUsed() / (1024.0f * 1024.0f));

	//// Optional: ECS stats
	//const auto& ecs = ECS::GetStats();
	//ImGui::SeparatorText("ECS");
	//ImGui::Text("Entities: %d", ecs.entity_count);
	//ImGui::Text("Systems:  %d", ecs.system_count);
	int a = 10;
}

void DebugLayer::InitWindow()
{
	m_frame_times.resize(FrametimeGraphSize, 0.f);
	m_x_axe_values.resize(FrametimeGraphSize, 0.f);

	for (auto i = 0; i < m_x_axe_values.size(); i++)
	{
		m_x_axe_values[i] = -5.f + 5.f / static_cast<float>(FrametimeGraphSize) * static_cast<float>(i);
	}
}

void DebugLayer::RenderFPSSection(DeltaTime dt)
{
	static float time_accum = 0.f;
	static int frame_count = 0;
	static float fps = 0.f;
	static float ms = 0.f;
	static float frametime_1 = 0.f;
	static float frametime_01 = 0.f;

	// Calculate heavy processing in background to not slow down main loop
	// Order of submiting frames is reserved so eventually all frametimes buffer members are calculated
	// Practically only few last frames will data race, so it wont impact avg stats but greatly reduce CPU load
	DaedalusThreads::Inst().Submit(
		[&]()
		{
			time_accum += dt.GetMilliseconds();
			frame_count++;

			if (time_accum >= 0.025f * MilliMult)
			{
				fps = MilliMult * static_cast<float>(frame_count) / time_accum;
				ms = MilliMult / fps;
				time_accum = 0.0f;
				frame_count = 0.f;

				static int frame_index = 0;
				if (frame_index < m_frame_times.size())
				{
					m_frame_times[frame_index++] = ms;
				}
				else
				{
					std::move(m_frame_times.begin() + 1, m_frame_times.end(), m_frame_times.begin());
					m_frame_times.back() = ms;
				}
			}

			frametime_1 = CalculateLowPercentile(0.01f);
			frametime_01 = CalculateLowPercentile(0.001f);
		});

	ImGui::Text("Average FPS: %.2f", fps);
	ImGui::Text("1%% FPS: %.2f", MilliMult / frametime_1);
	ImGui::Text("0.1%% FPS: %.2f", MilliMult / frametime_01);
	ImGui::Text("Frame Time: %.2f ms", ms);

	constexpr float plot_height = 300.0f;
	constexpr float plot_width = 500.0f;
	constexpr float plot_max = 50.0f;
	constexpr float plot_min = 0.0f;

	if (ImPlot::BeginPlot("Frame Time", ImVec2(plot_width, plot_height), ImPlotFlags_NoLegend))
	{
		ImPlot::SetupAxes("Realtime (s)", "Frametime (ms)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxesLimits(-5.f, 0.f, plot_min, plot_max, ImPlotCond_Once);

		ImPlot::PlotLine("Frame Time", m_x_axe_values.data(), m_frame_times.data(), static_cast<int>(m_frame_times.size()));

		ImPlot::EndPlot();
	}
}

float DebugLayer::CalculateLowPercentile(float percentile) const
{
	if (m_frame_times.empty())
		return 1.0f;

	std::vector<float> sorted = m_frame_times;
	std::sort(sorted.begin(), sorted.end());

	size_t count = static_cast<size_t>(percentile * sorted.size());
	count = std::max(count, size_t(1));

	float sum = 0.0f;
	for (size_t i = 1; i <= count; ++i)
		sum += sorted[sorted.size() - i];

	return sum > 0.f ? sum / static_cast<float>(count) : 1.f;
}