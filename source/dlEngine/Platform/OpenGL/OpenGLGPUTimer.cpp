#include "dlpch.h"
#include "OpenGLGPUTimer.h"

#include "OpenGLHeaders.h"

using namespace Daedalus;

#ifdef DL_PLATFORM_ANDROID
OpenGLGPUTimer::OpenGLGPUTimer()
{
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "GPU timestamps unavailable on this backend; CPU profiling remains enabled");
}
OpenGLGPUTimer::~OpenGLGPUTimer() = default;
void OpenGLGPUTimer::BeginFrame() {}
void OpenGLGPUTimer::EndFrame() {}
void OpenGLGPUTimer::BeginScope(const char*) {}
void OpenGLGPUTimer::EndScope() {}
void OpenGLGPUTimer::Resolve(Frame&) {}
#else
OpenGLGPUTimer::OpenGLGPUTimer()
{
	for (auto& frame : m_frames)
		glGenQueries(static_cast<GLsizei>(frame.queries.size()), frame.queries.data());
}

OpenGLGPUTimer::~OpenGLGPUTimer()
{
	for (auto& frame : m_frames)
		glDeleteQueries(static_cast<GLsizei>(frame.queries.size()), frame.queries.data());
}

void OpenGLGPUTimer::BeginFrame()
{
	m_current = (m_current + 1) % FramesInFlight;
	auto& frame = m_frames[m_current];
	if (frame.pending)
		Resolve(frame);

	frame.scope_count = 0;
	frame.pending = false;
	m_depth = 0;
	m_in_frame = Profiler::IsEnabled();
	if (m_in_frame)
	{
		// Calibrate GPU timestamps against the profiler clock so GPU scopes line up with CPU scopes in traces
		GLint64 gpu_now_ns = 0;
		glGetInteger64v(GL_TIMESTAMP, &gpu_now_ns);
		frame.gpu_to_cpu_offset_us = Profiler::NowMicroseconds() - gpu_now_ns / 1000;
	}
}

void OpenGLGPUTimer::EndFrame()
{
	if (!m_in_frame)
		return;

	// Close scopes left open so their queries are always issued
	while (m_depth > 0)
		EndScope();

	m_frames[m_current].pending = m_frames[m_current].scope_count > 0;
	m_in_frame = false;
}

void OpenGLGPUTimer::BeginScope(const char* name)
{
	auto& frame = m_frames[m_current];
	if (!m_in_frame || frame.scope_count >= MaxScopes || m_depth >= MaxDepth)
	{
		if (m_in_frame && m_depth < MaxDepth)
			m_open[m_depth++] = MaxScopes; // Placeholder so EndScope stays balanced
		return;
	}

	const auto index = frame.scope_count++;
	auto& scope = frame.scopes[index];
	scope.name = name;
	scope.depth = static_cast<uint32_t>(m_depth);
	scope.begin_query = frame.queries[1 + index * 2];
	scope.end_query = frame.queries[2 + index * 2];
	glQueryCounter(scope.begin_query, GL_TIMESTAMP);
	m_open[m_depth++] = index;
}

void OpenGLGPUTimer::EndScope()
{
	if (!m_in_frame || m_depth == 0)
		return;

	const auto index = m_open[--m_depth];
	if (index < MaxScopes)
		glQueryCounter(m_frames[m_current].scopes[index].end_query, GL_TIMESTAMP);
}

void OpenGLGPUTimer::Resolve(Frame& frame)
{
	frame.pending = false;

	// Never block: if the oldest frame is still not finished on the GPU, drop it
	GLint available = 0;
	glGetQueryObjectiv(frame.scopes[frame.scope_count - 1].end_query, GL_QUERY_RESULT_AVAILABLE, &available);
	if (!available)
	{
		Profiler::ReportDroppedGpuFrame();
		return;
	}

	const size_t count = frame.scope_count;
	for (size_t i = 0; i < count; ++i)
	{
		const auto& scope = frame.scopes[i];
		GLuint64 begin = 0;
		GLuint64 end = 0;
		glGetQueryObjectui64v(scope.begin_query, GL_QUERY_RESULT, &begin);
		glGetQueryObjectui64v(scope.end_query, GL_QUERY_RESULT, &end);

		// Convert both endpoints, not start and duration separately; separate rounding lets a child end past its parent
		const auto begin_us = static_cast<int64_t>(begin / 1000) + frame.gpu_to_cpu_offset_us;
		const GLuint64 clamped_end = end > begin ? end : begin;
		const auto end_us = static_cast<int64_t>(clamped_end / 1000) + frame.gpu_to_cpu_offset_us;

		auto& sample = m_resolved[i];
		sample.name = scope.name;
		sample.start_us = begin_us;
		sample.duration_us = end_us - begin_us;
		sample.thread_id = Profiler::GpuThreadId;
		sample.depth = scope.depth;
	}

	// The clock offset is re-measured every frame and jitters by a few microseconds; GPU work is serial,
	// so shift this frame forward if it would start before the previous one ended (keeps trace slices disjoint)
	if (count > 0 && m_resolved[0].start_us < m_last_gpu_end_us)
	{
		const auto shift = m_last_gpu_end_us - m_resolved[0].start_us;
		for (size_t i = 0; i < count; ++i)
			m_resolved[i].start_us += shift;
	}
	for (size_t i = 0; i < count; ++i)
	{
		const int64_t sample_end = m_resolved[i].start_us + m_resolved[i].duration_us;
		if (sample_end > m_last_gpu_end_us)
			m_last_gpu_end_us = sample_end;
	}

	Profiler::SubmitGpuSamples(m_resolved.data(), count);
}
#endif
