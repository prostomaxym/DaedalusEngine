#pragma once

#include "Macros.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <vector>

namespace Daedalus
{
	struct ProfileSample
	{
		// Non-owning; must point to static storage (string literal or __func__)
		const char* name = nullptr;
		int64_t start_us = 0;
		int64_t duration_us = 0;
		uint32_t thread_id = 0;
		uint32_t depth = 0;
	};

	struct ProfileFrame
	{
		std::vector<ProfileSample> samples;
		// Most recent resolved GPU frame; lags CPU frames by a few frames
		std::vector<ProfileSample> gpu_samples;
		uint64_t index = 0;
		int64_t start_us = 0;
		int64_t duration_us = 0;
		uint32_t dropped_samples = 0;
	};

	// Rolling statistics of one named scope; see ProfilerStats
	struct ProfilerStatsEntry
	{
		static constexpr size_t HistorySize = 240;

		const char* name = nullptr; // Non-owning; points to the sample's static name
		std::array<float, HistorySize> history_ms{}; // Ring buffer, next write at history_head
		size_t history_head = 0;
		size_t history_count = 0;
		float last_ms = 0.f;
		float avg_ms = 0.f;
		float max_ms = 0.f;
		float p95_ms = 0.f;
		float p99_ms = 0.f;
		uint32_t last_calls = 0;

		// Copies history oldest-to-newest into out (must hold HistorySize floats); returns count
		size_t CopyHistory(float* out) const
		{
			const size_t start = (history_head + HistorySize - history_count) % HistorySize;
			for (size_t i = 0; i < history_count; ++i)
				out[i] = history_ms[(start + i) % HistorySize];
			return history_count;
		}
	};

	// CPU scoped-timer profiler. Samples may be submitted from any thread; frames are delimited
	// by BeginFrame/EndFrame on the main thread. Buffers are preallocated so steady-state
	// profiling does not allocate (except while a Chrome trace capture is recording).
	class DAEDALUS_API Profiler
	{
	public:
		static constexpr size_t MaxSamplesPerFrame = 8192;
		static constexpr size_t MaxCaptureSamples = 4'000'000;
		static constexpr size_t MaxGpuSamplesPerFrame = 256;
		// Thread id used for GPU samples in traces
		static constexpr uint32_t GpuThreadId = 1'000'000;

		Profiler() = delete;

		static void SetEnabled(bool enabled);
		[[nodiscard]] static bool IsEnabled();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(const ProfileSample& sample);
		// Replaces the latest GPU frame; start_us must already be on the profiler clock
		static void SubmitGpuSamples(const ProfileSample* samples, size_t count);
		// Called when GPU results were not ready in time and a frame of GPU samples was discarded
		static void ReportDroppedGpuFrame();
		[[nodiscard]] static uint64_t GetDroppedGpuFrames();

		// Copies the last completed frame into out, reusing its capacity
		static void CopyLastFrame(ProfileFrame& out);
		// Rolling per-scope stats, updated every frame independent of any UI
		static void CopyStats(std::vector<ProfilerStatsEntry>& cpu, std::vector<ProfilerStatsEntry>& gpu);

		static void StartCapture();
		[[nodiscard]] static bool IsCapturing();
		[[nodiscard]] static size_t GetCaptureSampleCount();
		// Stops recording and writes a Chrome trace (chrome://tracing, Perfetto) to file
		static bool StopCapture(const std::filesystem::path& file);

		static void WriteChromeTrace(std::ostream& stream, const std::vector<ProfileSample>& samples);

		// Clears all frame and capture state; intended for tests
		static void Reset();

		[[nodiscard]] static int64_t NowMicroseconds();
		[[nodiscard]] static uint32_t CurrentThreadId();
	};

	class DAEDALUS_API ProfileScope
	{
	public:
		explicit ProfileScope(const char* name);
		~ProfileScope();

		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;
		ProfileScope(ProfileScope&&) = delete;
		ProfileScope& operator=(ProfileScope&&) = delete;

	private:
		const char* m_name;
		int64_t m_start_us = 0;
		uint32_t m_depth = 0;
		bool m_active = false;
	};
}

#define DL_PROFILE_CONCAT_IMPL(a, b) a##b
#define DL_PROFILE_CONCAT(a, b) DL_PROFILE_CONCAT_IMPL(a, b)

#ifdef DL_DEV_TOOLS
	#define DL_PROFILE_SCOPE(name) ::Daedalus::ProfileScope DL_PROFILE_CONCAT(dl_profile_scope_, __LINE__)(name)
	#define DL_PROFILE_FUNCTION() DL_PROFILE_SCOPE(__func__)
#else
	#define DL_PROFILE_SCOPE(name)
	#define DL_PROFILE_FUNCTION()
#endif
