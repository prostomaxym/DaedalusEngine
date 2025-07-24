#include "dlpch.h"

#include "DebugLayer.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>

using namespace Daedalus;

namespace
{
	constexpr auto NanoMult = 1000000000.f;
	constexpr auto MilliMult = 1000.f;
}
void DebugLayer::Update(DeltaTime dt)
{
	RenderFPSSection(dt);

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
}

void DebugLayer::ConstructWindow()
{
	m_frame_times.fill(0.f);
}

void DebugLayer::RenderFPSSection(DeltaTime dt)
{
	static double time_accum = 0.0;
	static int frame_count = 0;
	static double fps = 0.0;
	static double ms = 0.0;

	time_accum += dt.GetNanoseconds();
	frame_count++;

	if (time_accum >= 0.1f * NanoMult)
	{
		fps = NanoMult * static_cast<double>(frame_count) / time_accum;
		ms = MilliMult / fps;
		time_accum = 0.0f;
		frame_count = 0;
	}

	static int frame_index = 0;

	m_frame_times[frame_index] = ms;
	frame_index = (frame_index + 1) % m_frame_times.size();

	const auto frametime_1 = CalculateLowPercentile(0.01f);
	const auto frametime_01 = CalculateLowPercentile(0.001f);

	ImGui::Text("Average FPS: %.2f", fps);
	ImGui::Text("1%% FPS: %.2f", MilliMult / frametime_1);
	ImGui::Text("0.1%% FPS: %.2f", MilliMult / frametime_01);
	ImGui::Text("Frame Time: %.2f ms", ms);

	const float plot_height = 200.0f;
	const float plot_width = 400.0f;
	const float plot_max = 50.0f;
	const float plot_min = 0.0f;

	ImGui::BeginGroup();
	ImGui::Text("ms");
	ImGui::Text("%.0f", plot_max);
	ImGui::Dummy(ImVec2(0, plot_height / 2 - ImGui::GetTextLineHeight() / 2));
	ImGui::Text("%.0f", (plot_max + plot_min) / 2);
	ImGui::Dummy(ImVec2(0, plot_height / 2 - ImGui::GetTextLineHeight() / 2));
	ImGui::Text("%.0f", plot_min);
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::PlotLines(
		"##frametime", m_frame_times.data(), static_cast<int>(m_frame_times.size()),
		frame_index, nullptr, plot_min, plot_max, ImVec2(plot_width, plot_height)
	);
}

float DebugLayer::CalculateLowPercentile(float percentile) const
{
	if (m_frame_times.empty())
		return 1.0f;

	std::array<float, FrametimeGraphSize> sorted = m_frame_times;
	std::sort(sorted.begin(), sorted.end());

	size_t count = static_cast<size_t>(percentile * sorted.size());
	count = std::max(count, size_t(1));

	float sum = 0.0f;
	for (size_t i = 0; i < count; ++i)
		sum += sorted[i];

	return sum > 0.f ? sum / static_cast<float>(count) : 1.f;
}