#pragma once

#include "Macros.h"

#include "Utils/RingVector.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <utility>
#include <mutex>

namespace Daedalus {

	class DAEDALUS_API Log
	{
		friend class DebugLayer;
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

		static const char* ToString(Log::Categories category)
		{
			switch (category)
			{
			case Log::Categories::EngineCore: return "EngineCore";
			case Log::Categories::ECS:        return "ECS";
			case Log::Categories::Renderer:   return "Renderer";
			case Log::Categories::Events:     return "Events";
			case Log::Categories::Platform:   return "Platform";
			case Log::Categories::Application:return "Application";
			default:                         return "UnknownCategory";
			}
		}

		static const char* ToString(Log::Levels level)
		{
			switch (level)
			{
			case Log::Levels::Trace:    return "Trace";
			case Log::Levels::Info:     return "Info";
			case Log::Levels::Warn:     return "Warn";
			case Log::Levels::Error:    return "Error";
			case Log::Levels::Critical: return "Critical";
			default:                   return "UnknownLevel";
			}
		}

	private:
		struct Entry
		{
			std::chrono::system_clock::time_point timestamp;
			Levels level;
			Categories category;
			std::string message;
		};

		class Storage
		{
		public:
			static const int MaxEntrySize = 1000;
		public:
			static void Add(Entry entry)
			{
				std::lock_guard lock(s_mutex);
				s_entries.push(std::move(entry));
			}

			static const RingVector<Entry, MaxEntrySize>& GetEntries()
			{
				return s_entries;
			}

			static void Clear()
			{
				std::lock_guard lock(s_mutex);
				s_entries.clear();
			}

		private:
			static inline RingVector<Entry, MaxEntrySize> s_entries;
			static inline std::mutex s_mutex;
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
			if (!IsEnabled(category))
			{
				SkipProcessing();
				return;
			}

			const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);

			if (IsDevBuild() && !s_stop_updating)
			{
				Storage::Add({
					std::chrono::system_clock::now(),
					level,
					category,
					msg
					});
			}

			switch (level)
			{
				case Log::Levels::Trace:    TraceImpl(msg); break;
				case Log::Levels::Info:     InfoImpl(msg); break;
				case Log::Levels::Warn:     WarnImpl(msg); break;
				case Log::Levels::Error:    ErrorImpl(msg); break;
				case Log::Levels::Critical: CriticalImpl(msg); break;
				default: break;
			}
		}

	private:
		static void TraceImpl(const std::string& msg);
		static void InfoImpl(const std::string& msg);
		static void WarnImpl(const std::string& msg);
		static void ErrorImpl(const std::string& msg);
		static void CriticalImpl(const std::string& msg);

		static int& GetAllowedCategories() { return s_allowed_categories; }
		static bool& GetLockUpdating() { return s_stop_updating; }

		static int s_allowed_categories;
		static bool s_stop_updating;
	};
}