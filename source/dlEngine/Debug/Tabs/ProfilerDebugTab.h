#pragma once

#include "Debug/Profiler.h"
#include "Utils/RingVector.h"

#include <array>
#include <string_view>
#include <vector>

namespace Daedalus
{
	class ProfilerDebugTab
	{
	public:
		void Init();
		void Render();

	private:
		static constexpr int GraphSize = 500;
		static constexpr float ProfilerTextRefreshSec = 0.5f;
		void RenderProfilerStats(const char* id, const std::vector<ProfilerStatsEntry>& entries);

		std::vector<const ProfileSample*> m_profiler_sorted_samples;
		std::vector<const ProfilerStatsEntry*> m_profiler_sorted_stats;
		ProfileFrame m_profile_frame;
		ProfileFrame m_profile_live_frame;
		float m_profiler_refresh_accum = 0.f;
		RingVector<float, GraphSize> m_profile_frame_times;
		bool m_profiler_paused = false;
		std::vector<ProfilerStatsEntry> m_profiler_cpu_stats;
		std::vector<ProfilerStatsEntry> m_profiler_gpu_stats;
		std::string_view m_profiler_selected_scope; // Points to static scope-name storage
		std::array<float, ProfilerStatsEntry::HistorySize> m_profiler_history{};
	};
}
