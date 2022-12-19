#pragma once

#include "Daedalus/Core/Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Daedalus {

class DAEDALUS_API Log
{
public:
	static void Init();

	inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_core_logger; }
	inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return m_client_logger; }

private:
	static std::shared_ptr<spdlog::logger> m_core_logger;
	static std::shared_ptr<spdlog::logger> m_client_logger;
};

}

// CORE LOG LEVELS
#define DL_CORE_TRACE(...)		::Daedalus::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define DL_CORE_INFO(...)		::Daedalus::Log::GetCoreLogger()->info(__VA_ARGS__)
#define DL_CORE_WARN(...)		::Daedalus::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define DL_CORE_ERROR(...)		::Daedalus::Log::GetCoreLogger()->error(__VA_ARGS__)
#define DL_CORE_CRITICAL(...)	::Daedalus::Log::GetCoreLogger()->critical(__VA_ARGS__)

// CLIENT LOG LEVELS
#define DL_TRACE(...)			::Daedalus::Log::GetClientLogger()->trace(__VA_ARGS__)
#define DL_INFO(...)			::Daedalus::Log::GetClientLogger()->info(__VA_ARGS__)
#define DL_WARN(...)		    ::Daedalus::Log::GetClientLogger()->warn(__VA_ARGS__)
#define DL_ERROR(...)			::Daedalus::Log::GetClientLogger()->error(__VA_ARGS__)
#define DL_CRITICAL(...)		::Daedalus::Log::GetClientLogger()->critical(__VA_ARGS__)