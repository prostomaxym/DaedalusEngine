#include "dlpch.h"

#include "Profiler.h"
#include "ProfilerStats.h"

#include "Log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>

using namespace Daedalus;

namespace
{
	struct ProfilerState
	{
		ProfilerState()
		{
			current.reserve(Profiler::MaxSamplesPerFrame);
			last.samples.reserve(Profiler::MaxSamplesPerFrame);
			last.gpu_samples.reserve(Profiler::MaxGpuSamplesPerFrame);
		}

		std::mutex mutex;
		std::vector<ProfileSample> current;
		ProfileFrame last;
		uint32_t current_dropped = 0;
		int64_t frame_start_us = 0;
		uint64_t frame_index = 0;

		bool capturing = false;
		std::vector<ProfileSample> capture;

		ProfilerStats cpu_stats;
		ProfilerStats gpu_stats;
	};

	ProfilerState& GetState()
	{
		static ProfilerState state;
		return state;
	}

	std::atomic<bool> s_enabled{ true };
	std::atomic<uint64_t> s_dropped_gpu_frames{ 0 };
	std::atomic<uint32_t> s_next_thread_id{ 0 };
	thread_local uint32_t t_depth = 0;

	const auto s_epoch = std::chrono::steady_clock::now();

	constexpr const char* FrameSampleName = "Frame";

	void WriteEscaped(std::ostream& stream, const char* text)
	{
		for (const char* c = text ? text : ""; *c; ++c)
		{
			switch (*c)
			{
			case '"':  stream << "\\\""; break;
			case '\\': stream << "\\\\"; break;
			case '\n': stream << "\\n"; break;
			case '\t': stream << "\\t"; break;
			default:
				if (static_cast<unsigned char>(*c) < 0x20)
					stream << ' ';
				else
					stream << *c;
			}
		}
	}

	void AppendToCapture(ProfilerState& state, const ProfileSample* begin, const ProfileSample* end)
	{
		const size_t available = Profiler::MaxCaptureSamples - std::min(state.capture.size(), Profiler::MaxCaptureSamples);
		const size_t count = std::min(static_cast<size_t>(end - begin), available);
		state.capture.insert(state.capture.end(), begin, begin + count);
	}
}

void Profiler::SetEnabled(bool enabled)
{
	s_enabled.store(enabled, std::memory_order_relaxed);
}

bool Profiler::IsEnabled()
{
	return s_enabled.load(std::memory_order_relaxed);
}

void Profiler::BeginFrame()
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	state.frame_start_us = NowMicroseconds();
}

void Profiler::EndFrame()
{
	auto& state = GetState();
	const auto now = NowMicroseconds();

	std::lock_guard lock(state.mutex);
	state.last.samples.swap(state.current);
	state.current.clear();
	state.last.index = state.frame_index++;
	state.last.start_us = state.frame_start_us;
	state.last.duration_us = now - state.frame_start_us;
	state.last.dropped_samples = state.current_dropped;
	state.current_dropped = 0;

	if (IsEnabled())
		state.cpu_stats.AddFrame(state.last.samples);

	if (state.capturing)
	{
		const ProfileSample frame_sample{ FrameSampleName, state.last.start_us, state.last.duration_us, CurrentThreadId(), 0 };
		AppendToCapture(state, &frame_sample, &frame_sample + 1);
		AppendToCapture(state, state.last.samples.data(), state.last.samples.data() + state.last.samples.size());
	}
}

void Profiler::Submit(const ProfileSample& sample)
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	if (state.current.size() < MaxSamplesPerFrame)
		state.current.push_back(sample);
	else
		++state.current_dropped;
}

void Profiler::SubmitGpuSamples(const ProfileSample* samples, size_t count)
{
	count = std::min(count, MaxGpuSamplesPerFrame);
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	state.last.gpu_samples.assign(samples, samples + count);
	for (auto& sample : state.last.gpu_samples)
		sample.thread_id = GpuThreadId;
	state.gpu_stats.AddFrame(state.last.gpu_samples);

	if (state.capturing)
		AppendToCapture(state, state.last.gpu_samples.data(), state.last.gpu_samples.data() + state.last.gpu_samples.size());
}

void Profiler::ReportDroppedGpuFrame()
{
	s_dropped_gpu_frames.fetch_add(1, std::memory_order_relaxed);
}

uint64_t Profiler::GetDroppedGpuFrames()
{
	return s_dropped_gpu_frames.load(std::memory_order_relaxed);
}

void Profiler::CopyLastFrame(ProfileFrame& out)
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	out.samples.assign(state.last.samples.begin(), state.last.samples.end());
	out.gpu_samples.assign(state.last.gpu_samples.begin(), state.last.gpu_samples.end());
	out.index = state.last.index;
	out.start_us = state.last.start_us;
	out.duration_us = state.last.duration_us;
	out.dropped_samples = state.last.dropped_samples;
}

void Profiler::CopyStats(std::vector<ProfilerStats::Entry>& cpu, std::vector<ProfilerStats::Entry>& gpu)
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	cpu.assign(state.cpu_stats.GetEntries().begin(), state.cpu_stats.GetEntries().end());
	gpu.assign(state.gpu_stats.GetEntries().begin(), state.gpu_stats.GetEntries().end());
}

void Profiler::StartCapture()
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	state.capture.clear();
	state.capturing = true;
}

bool Profiler::IsCapturing()
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	return state.capturing;
}

size_t Profiler::GetCaptureSampleCount()
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	return state.capture.size();
}

bool Profiler::StopCapture(const std::filesystem::path& file)
{
	std::vector<ProfileSample> samples;
	{
		auto& state = GetState();
		std::lock_guard lock(state.mutex);
		if (!state.capturing)
			return false;
		state.capturing = false;
		samples.swap(state.capture);
	}

	std::error_code ec;
	if (file.has_parent_path())
		std::filesystem::create_directories(file.parent_path(), ec);

	std::ofstream stream(file, std::ios::out | std::ios::trunc);
	if (!stream)
	{
		Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "Profiler: failed to open trace file {0}", file.string());
		return false;
	}

	WriteChromeTrace(stream, samples);
	if (!stream)
	{
		Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "Profiler: failed to write trace file {0}", file.string());
		return false;
	}

	Log::Write(Log::Levels::Info, Log::Categories::EngineCore, "Profiler: wrote {0} samples to {1}", samples.size(), file.string());
	return true;
}

void Profiler::WriteChromeTrace(std::ostream& stream, const std::vector<ProfileSample>& samples)
{
	stream << "{\"displayTimeUnit\":\"ms\",\"traceEvents\":[";
	bool first = true;
	for (const auto& sample : samples)
	{
		if (!first)
			stream << ',';
		first = false;

		stream << "{\"name\":\"";
		WriteEscaped(stream, sample.name);
		stream << "\",\"cat\":\"cpu\",\"ph\":\"X\",\"ts\":" << sample.start_us
			<< ",\"dur\":" << sample.duration_us
			<< ",\"pid\":1,\"tid\":" << sample.thread_id << '}';
	}
	const bool has_gpu = std::any_of(samples.begin(), samples.end(), [](const ProfileSample& s) { return s.thread_id == GpuThreadId; });
	if (has_gpu)
	{
		if (!first)
			stream << ',';
		stream << "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":1,\"tid\":" << GpuThreadId << ",\"args\":{\"name\":\"GPU\"}}";
	}
	stream << "]}";
}

void Profiler::Reset()
{
	auto& state = GetState();
	std::lock_guard lock(state.mutex);
	state.current.clear();
	state.last.samples.clear();
	state.last.gpu_samples.clear();
	state.cpu_stats.Reset();
	state.gpu_stats.Reset();
	state.last.index = 0;
	state.last.start_us = 0;
	state.last.duration_us = 0;
	state.last.dropped_samples = 0;
	state.current_dropped = 0;
	state.frame_start_us = 0;
	state.frame_index = 0;
	state.capturing = false;
	state.capture.clear();
	s_dropped_gpu_frames.store(0, std::memory_order_relaxed);
}

int64_t Profiler::NowMicroseconds()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s_epoch).count();
}

uint32_t Profiler::CurrentThreadId()
{
	thread_local const uint32_t id = s_next_thread_id.fetch_add(1, std::memory_order_relaxed);
	return id;
}

ProfileScope::ProfileScope(const char* name)
	: m_name(name)
{
	if (!Profiler::IsEnabled())
		return;

	m_active = true;
	m_depth = t_depth++;
	m_start_us = Profiler::NowMicroseconds();
}

ProfileScope::~ProfileScope()
{
	if (!m_active)
		return;

	const auto end_us = Profiler::NowMicroseconds();
	--t_depth;
	Profiler::Submit({ m_name, m_start_us, end_us - m_start_us, Profiler::CurrentThreadId(), m_depth });
}
