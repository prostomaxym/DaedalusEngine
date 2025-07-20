#pragma once

#include "Core/Core.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <utility>

namespace Daedalus {

	class DAEDALUS_API Log
	{
	public:
		enum Categories
		{
			EngineCore = BITMASK(0),
			ECS = BITMASK(1),
			Renderer = BITMASK(2),
			Events = BITMASK(3),
			Platform = BITMASK(4),
			Application = BITMASK(5)
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

		static void EnableAllCategories();
		static void EnableCategory(Categories category) { s_allowed_categories |= category; }
		static void DisableCategory(Categories category) { s_allowed_categories ^= category; }
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
					Log::TraceImpl(fmt::format(fmt, std::forward<Args>(args)...));
					break;
				case Daedalus::Log::Levels::Info:
					Log::InfoImpl(fmt::format(fmt, std::forward<Args>(args)...));
					break;
				case Daedalus::Log::Levels::Warn:
					Log::WarnImpl(fmt::format(fmt, std::forward<Args>(args)...));
					break;
				case Daedalus::Log::Levels::Error:
					Log::ErrorImpl(fmt::format(fmt, std::forward<Args>(args)...));
					break;
				case Daedalus::Log::Levels::Critical:
					Log::CriticalImpl(fmt::format(fmt, std::forward<Args>(args)...));
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
		static void TraceImpl(const std::string& msg);
		static void InfoImpl(const std::string& msg);
		static void WarnImpl(const std::string& msg);
		static void ErrorImpl(const std::string& msg);
		static void CriticalImpl(const std::string& msg);

		static int s_allowed_categories;
	};

}