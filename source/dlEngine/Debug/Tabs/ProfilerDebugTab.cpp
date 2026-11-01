#include "dlpch.h"
#include "ProfilerDebugTab.h"
#include "Config/PathConfig.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <implot.h>

using namespace Daedalus;

namespace
{
	constexpr auto MilliMult = 1000.f;
}

void ProfilerDebugTab::Init()
{
	m_profile_frame_times.resize(GraphSize, 0.f);
	m_profile_frame.samples.reserve(Profiler::MaxSamplesPerFrame);
	m_profile_live_frame.samples.reserve(Profiler::MaxSamplesPerFrame);
}

void ProfilerDebugTab::Render()
{


	bool enabled = Profiler::IsEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
		Profiler::SetEnabled(enabled);

	ImGui::SameLine();
	ImGui::Checkbox("Pause view", &m_profiler_paused);

	// Task Manager style: holding Ctrl freezes the view so values can be read.
	const bool ctrl_held = ImGui::GetIO().KeyCtrl;
	const bool paused = m_profiler_paused || ctrl_held;
	if (ctrl_held && !m_profiler_paused)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "(paused while Ctrl held)");
	}
	else if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Hold Ctrl to pause temporarily");

	ImGui::SameLine();
	if (!Profiler::IsCapturing())
	{
		if (ImGui::Button("Start Chrome trace capture"))
			Profiler::StartCapture();
	}
	else
	{
		if (ImGui::Button("Stop capture and save"))
		{
			const auto stamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			Profiler::StopCapture(PathConfig::GetLogsPath() / fmt::format("profile_{0}.json", stamp));
		}
		ImGui::SameLine();
		ImGui::Text("Recording: %zu samples", Profiler::GetCaptureSampleCount());
	}

	if (!paused)
	{
		Profiler::CopyLastFrame(m_profile_live_frame);
		m_profile_frame_times.push_back(static_cast<float>(m_profile_live_frame.duration_us) / MilliMult);
		m_profiler_refresh_accum += ImGui::GetIO().DeltaTime;
	}

	// Text is refreshed at a readable rate; a per-frame refresh makes it flicker.
	if (!paused && m_profiler_refresh_accum >= ProfilerTextRefreshSec)
	{
		m_profiler_refresh_accum = 0.f;
		Profiler::CopyLastFrame(m_profile_frame);
		std::sort(m_profile_frame.samples.begin(), m_profile_frame.samples.end(),
			[](const ProfileSample& a, const ProfileSample& b)
			{
				if (a.thread_id != b.thread_id)
					return a.thread_id < b.thread_id;
				return a.start_us < b.start_us;
			});
		Profiler::CopyStats(m_profiler_cpu_stats, m_profiler_gpu_stats);
	}

	ImGui::Text("Frame #%llu: %.3f ms, %zu samples, %u dropped",
		static_cast<unsigned long long>(m_profile_frame.index),
		static_cast<float>(m_profile_frame.duration_us) / MilliMult,
		m_profile_frame.samples.size(),
		m_profile_frame.dropped_samples);
	ImGui::SameLine();
	ImGui::Text("| GPU frames dropped: %llu", static_cast<unsigned long long>(Profiler::GetDroppedGpuFrames()));
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("GPU timer results that were not ready in time and were discarded (GPU more than 4 frames behind)");

	if (ImPlot::BeginPlot("Profiled frame time", ImVec2(900.f, 300.f)))
	{
		const auto data = m_profile_frame_times.linearize();
		const int count = static_cast<int>(data.size());
		ImPlot::SetupAxes("Frames ago", "ms", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("Frame", data.data(), count, 1.0, -static_cast<double>(count - 1));
		ImPlot::EndPlot();
	}

	if (!ImGui::BeginTabBar("ProfilerViews"))
	{
		return;
	}

	constexpr auto table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable
		| ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate;
	if (ImGui::BeginTabItem("Frame"))
	{
	if (ImGui::BeginTable("ProfilerSamples", 4, table_flags, ImVec2(900.f, 500.f)))
	{
		constexpr auto numeric = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending;
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed, 60.f);
		ImGui::TableSetupColumn("Duration (ms)", numeric, 110.f);
		ImGui::TableSetupColumn("% of frame", numeric, 90.f);
		ImGui::TableHeadersRow();

		m_profiler_sorted_samples.clear();
		for (const auto& sample : m_profile_frame.samples)
			m_profiler_sorted_samples.push_back(&sample);

		// Unsorted (tristate) keeps the call-hierarchy order with indentation.
		bool hierarchical = true;
		const ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
		if (specs && specs->SpecsCount > 0)
		{
			hierarchical = false;
			const auto& spec = specs->Specs[0];
			const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
			std::stable_sort(m_profiler_sorted_samples.begin(), m_profiler_sorted_samples.end(),
				[&spec, ascending](const ProfileSample* a, const ProfileSample* b)
				{
					int cmp = 0;
					switch (spec.ColumnIndex)
					{
					case 0: cmp = std::strcmp(a->name ? a->name : "", b->name ? b->name : ""); break;
					case 1: cmp = (a->thread_id > b->thread_id) - (a->thread_id < b->thread_id); break;
					default: cmp = (a->duration_us > b->duration_us) - (a->duration_us < b->duration_us); break;
					}
					return ascending ? cmp < 0 : cmp > 0;
				});
		}

		const float frame_ms = std::max(static_cast<float>(m_profile_frame.duration_us) / MilliMult, 0.001f);
		for (const ProfileSample* sample_ptr : m_profiler_sorted_samples)
		{
			const auto& sample = *sample_ptr;
			const float ms = static_cast<float>(sample.duration_us) / MilliMult;
			const float indent = hierarchical ? static_cast<float>(sample.depth) * 12.f + 1.f : 1.f;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Indent(indent);
			ImGui::TextUnformatted(sample.name ? sample.name : "<unnamed>");
			ImGui::Unindent(indent);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", sample.thread_id);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f", ms);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.1f", ms / frame_ms * 100.f);
		}
		ImGui::EndTable();
	}
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Scopes"))
	{
		RenderProfilerStats("CpuScopes", m_profiler_cpu_stats);
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("GPU"))
	{
		if (m_profiler_gpu_stats.empty())
			ImGui::TextUnformatted("No GPU samples yet (results arrive a few frames late).");
		else
			RenderProfilerStats("GpuScopes", m_profiler_gpu_stats);
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

void ProfilerDebugTab::RenderProfilerStats(const char* id, const std::vector<ProfilerStatsEntry>& entries)
{
	constexpr auto table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable
		| ImGuiTableFlags_Sortable;
	const ProfilerStatsEntry* selected_entry = nullptr;
	if (ImGui::BeginTable(id, 7, table_flags, ImVec2(900.f, 300.f)))
	{
		constexpr auto numeric = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending;
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Calls", numeric, 60.f);
		ImGui::TableSetupColumn("Last (ms)", numeric, 80.f);
		ImGui::TableSetupColumn("Avg (ms)", numeric | ImGuiTableColumnFlags_DefaultSort, 80.f);
		ImGui::TableSetupColumn("p95 (ms)", numeric, 80.f);
		ImGui::TableSetupColumn("p99 (ms)", numeric, 80.f);
		ImGui::TableSetupColumn("Max (ms)", numeric, 80.f);
		ImGui::TableHeadersRow();

		m_profiler_sorted_stats.clear();
		for (const auto& entry : entries)
			m_profiler_sorted_stats.push_back(&entry);

		const ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
		if (specs && specs->SpecsCount > 0)
		{
			const auto& spec = specs->Specs[0];
			const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
			const auto key = [&spec](const ProfilerStatsEntry* e) -> double
			{
				switch (spec.ColumnIndex)
				{
				case 1: return e->last_calls;
				case 2: return e->last_ms;
				case 3: return e->avg_ms;
				case 4: return e->p95_ms;
				case 5: return e->p99_ms;
				default: return e->max_ms;
				}
			};
			std::stable_sort(m_profiler_sorted_stats.begin(), m_profiler_sorted_stats.end(),
				[&spec, &key, ascending](const ProfilerStatsEntry* a, const ProfilerStatsEntry* b)
				{
					if (spec.ColumnIndex == 0)
					{
						const int cmp = std::strcmp(a->name ? a->name : "", b->name ? b->name : "");
						return ascending ? cmp < 0 : cmp > 0;
					}
					return ascending ? key(a) < key(b) : key(a) > key(b);
				});
		}

		for (const ProfilerStatsEntry* entry_ptr : m_profiler_sorted_stats)
		{
			const auto& entry = *entry_ptr;
			const char* name = entry.name ? entry.name : "<unnamed>";
			const bool selected = m_profiler_selected_scope == name;
			if (selected)
				selected_entry = &entry;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(name);
			if (ImGui::Selectable(name, selected, ImGuiSelectableFlags_SpanAllColumns))
				m_profiler_selected_scope = name;
			ImGui::PopID();
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", entry.last_calls);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f", entry.last_ms);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f", entry.avg_ms);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.3f", entry.p95_ms);
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.3f", entry.p99_ms);
			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%.3f", entry.max_ms);
		}
		ImGui::EndTable();
	}

	const auto* entry = selected_entry;
	if (!entry)
	{
		ImGui::TextUnformatted("Select a scope to see its history.");
		return;
	}

	const auto count = entry->CopyHistory(m_profiler_history.data());
	if (ImPlot::BeginPlot("Scope history", ImVec2(900.f, 300.f)))
	{
		ImPlot::SetupAxes("Frames ago", "ms", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine(entry->name ? entry->name : "<unnamed>", m_profiler_history.data(), static_cast<int>(count), 1.0, -static_cast<double>(count) + 1.0);
		ImPlot::EndPlot();
	}
}

