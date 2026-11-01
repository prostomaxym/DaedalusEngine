#pragma once

#include "Macros.h"
#include "Profiler.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Daedalus
{
	// Rolling per-scope statistics built from consecutive profiler frames.
	// Allocates only when a scope name is seen for the first time.
	class DAEDALUS_API ProfilerStats
	{
	public:
		using Entry = ProfilerStatsEntry;
		static constexpr size_t HistorySize = ProfilerStatsEntry::HistorySize;

		void AddFrame(const std::vector<ProfileSample>& samples);
		void Reset();

		[[nodiscard]] const std::vector<Entry>& GetEntries() const { return m_entries; }
		[[nodiscard]] const Entry* Find(std::string_view name) const;

		// Nearest-rank percentile (0..1) of the first count values; reorders values
		[[nodiscard]] static float Percentile(float* values, size_t count, float percentile);

	private:
		std::array<float, HistorySize> m_scratch{};
		std::vector<Entry> m_entries;
		std::vector<int64_t> m_frame_us;
		std::vector<uint32_t> m_frame_calls;
		std::unordered_map<std::string_view, size_t> m_index;
	};
}
