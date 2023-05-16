#pragma once

#include "Daedalus/Core/Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <utility>

namespace Daedalus {

	class DAEDALUS_API Log
	{
	public:
		enum Categories
		{
			EngineCore = BITMASK(0),
			Renderer = BITMASK(1),
			Events = BITMASK(2),
			Platform = BITMASK(3)
		};

		enum class Levels
		{
			Trace,
			Info,
			Warn,
			Error,
			Critical
		};

	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_core_logger; }

		static void EnableAllCategories();
		static void EnableCategory(Categories category) { s_allowed_categories |= category; }
		static void DisableCategory(Categories category) { s_allowed_categories &= category; }
		static void ToggleCategory(Categories category, bool enable);
		static bool IsEnabled(Categories category) { return category & s_allowed_categories; }

		static void SkipProcessing() {}

		template<typename... Args>
		static void Write(Log::Levels level, Log::Categories category, fmt::format_string<Args...> fmt, Args &&... args)
		{
			if (IsEnabled(category))
			{
				switch (level)
				{
				case Daedalus::Log::Levels::Trace:
					GetCoreLogger()->trace(fmt, std::forward<Args>(args)...);
					break;
				case Daedalus::Log::Levels::Info:
					GetCoreLogger()->info(fmt, std::forward<Args>(args)...);
					break;
				case Daedalus::Log::Levels::Warn:
					GetCoreLogger()->warn(fmt, std::forward<Args>(args)...);
					break;
				case Daedalus::Log::Levels::Error:
					GetCoreLogger()->error(fmt, std::forward<Args>(args)...);
					break;
				case Daedalus::Log::Levels::Critical:
					GetCoreLogger()->critical(fmt, std::forward<Args>(args)...);
					break;
				default:
					break;
				}
			}
			else
			{
				SkipProcessing();
			}
		}

	private:
		static std::shared_ptr<spdlog::logger> s_core_logger;
		static int s_allowed_categories;
	};

}