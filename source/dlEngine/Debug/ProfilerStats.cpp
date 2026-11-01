#include "dlpch.h"

#include "ProfilerStats.h"

#include <algorithm>
#include <cmath>

using namespace Daedalus;

void ProfilerStats::AddFrame(const std::vector<ProfileSample>& samples)
{
	std::fill(m_frame_us.begin(), m_frame_us.end(), 0);
	std::fill(m_frame_calls.begin(), m_frame_calls.end(), 0u);

	for (const auto& sample : samples)
	{
		const char* raw_name = sample.name ? sample.name : "<unnamed>";
		const std::string_view name = raw_name;
		auto it = m_index.find(name);
		if (it == m_index.end())
		{
			Entry entry;
			entry.name = raw_name;
			m_entries.push_back(entry);
			m_frame_us.push_back(0);
			m_frame_calls.push_back(0);
			it = m_index.emplace(name, m_entries.size() - 1).first;
		}
		m_frame_us[it->second] += sample.duration_us;
		++m_frame_calls[it->second];
	}

	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		auto& entry = m_entries[i];
		entry.last_ms = static_cast<float>(m_frame_us[i]) / 1000.f;
		entry.last_calls = m_frame_calls[i];
		entry.history_ms[entry.history_head] = entry.last_ms;
		entry.history_head = (entry.history_head + 1) % HistorySize;
		entry.history_count = std::min(entry.history_count + 1, HistorySize);

		float sum = 0.f;
		float max = 0.f;
		for (size_t h = 0; h < entry.history_count; ++h)
		{
			const auto value = entry.history_ms[(entry.history_head + HistorySize - 1 - h) % HistorySize];
			sum += value;
			max = std::max(max, value);
		}
		entry.avg_ms = sum / static_cast<float>(entry.history_count);
		entry.max_ms = max;

		const auto count = entry.CopyHistory(m_scratch.data());
		entry.p95_ms = Percentile(m_scratch.data(), count, 0.95f);
		entry.p99_ms = Percentile(m_scratch.data(), count, 0.99f);
	}
}

void ProfilerStats::Reset()
{
	m_entries.clear();
	m_frame_us.clear();
	m_frame_calls.clear();
	m_index.clear();
}

const ProfilerStats::Entry* ProfilerStats::Find(std::string_view name) const
{
	const auto it = m_index.find(name);
	return it != m_index.end() ? &m_entries[it->second] : nullptr;
}

float ProfilerStats::Percentile(float* values, size_t count, float percentile)
{
	if (count == 0)
		return 0.f;

	percentile = std::clamp(percentile, 0.f, 1.f);
	const auto rank = static_cast<size_t>(std::ceil(percentile * static_cast<float>(count)));
	const size_t index = rank == 0 ? 0 : rank - 1;
	std::nth_element(values, values + index, values + count);
	return values[index];
}
