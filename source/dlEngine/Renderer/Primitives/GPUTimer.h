#pragma once

#include "Macros.h"

#include <memory>

namespace Daedalus
{
	// Measures GPU execution time of render scopes with asynchronous timestamp queries.
	// Results are read back a few frames later without stalling and forwarded to Profiler.
	// Graphics thread only.
	class DAEDALUS_API GPUTimer
	{
	public:
		static std::unique_ptr<GPUTimer> Create();

		virtual ~GPUTimer() = default;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		// name must point to static storage
		virtual void BeginScope(const char* name) = 0;
		virtual void EndScope() = 0;
	};
}
