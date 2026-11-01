#pragma once

#include "Renderer/Primitives/GPUTimer.h"
#include "Debug/Profiler.h"

#include <array>
#include <cstdint>

namespace Daedalus
{
	class OpenGLGPUTimer final : public GPUTimer
	{
	public:
		OpenGLGPUTimer();
		~OpenGLGPUTimer() override;

		OpenGLGPUTimer(const OpenGLGPUTimer&) = delete;
		OpenGLGPUTimer& operator=(const OpenGLGPUTimer&) = delete;
		OpenGLGPUTimer(OpenGLGPUTimer&&) = delete;
		OpenGLGPUTimer& operator=(OpenGLGPUTimer&&) = delete;

		void BeginFrame() override;
		void EndFrame() override;

		void BeginScope(const char* name) override;
		void EndScope() override;

	private:
		// Results are read FramesInFlight frames after submission
		static constexpr size_t FramesInFlight = 4;
		static constexpr size_t MaxScopes = 64;
		static constexpr size_t MaxDepth = 16;

		struct Scope
		{
			const char* name = nullptr;
			uint32_t depth = 0;
			uint32_t begin_query = 0;
			uint32_t end_query = 0;
		};

		struct Frame
		{
			std::array<uint32_t, MaxScopes * 2 + 1> queries{};
			std::array<Scope, MaxScopes> scopes{};
			size_t scope_count = 0;
			int64_t gpu_to_cpu_offset_us = 0; // Profiler clock minus GPU clock, sampled at frame begin
			bool pending = false;
		};

		void Resolve(Frame& frame);

		std::array<Frame, FramesInFlight> m_frames{};
		std::array<ProfileSample, MaxScopes> m_resolved{};
		std::array<size_t, MaxDepth> m_open{};
		size_t m_depth = 0;
		size_t m_current = 0;
		int64_t m_last_gpu_end_us = 0; // End of the last submitted GPU sample on the profiler clock
		bool m_in_frame = false;
	};
}
