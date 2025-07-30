#include "dlpch.h"
#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>


#include "Config/PathConfig.h"
#include "Platform/Platform.h"
using namespace Daedalus;

std::shared_ptr<spdlog::logger> s_impl = nullptr;
int Log::s_allowed_categories = 0;
bool Log::s_stop_updating = false;

namespace
{
	class LogFile
	{
	public:
		static LogFile& Inst()
		{
			static LogFile instance;
			return instance;
		}

		void Write(const std::string& msg, Log::Categories category)
		{
			std::lock_guard lock(mutex_);

			if (!stream_.is_open())
				CreateLogFile();

			if (stream_.is_open()) // Double if file is indeed open
			{
				const auto log_str = fmt::format("[{}][{}]: {}",
					Platform::FormatTimestamp(std::chrono::system_clock::now()),
					Log::ToString(category),
					msg);

				stream_ << log_str << '\n';
			}	
		}

		void Flush()
		{
			if (stream_.is_open())
				stream_.close();
		}

		bool IsEmpty()
		{
			return !stream_.is_open();
		}

	private:
		LogFile()
		{
			start_time_ = std::chrono::system_clock::now();
		}

		~LogFile()
		{
			Flush();
		}

		void CreateLogFile()
		{
			const auto log_folder = PathConfig::GetLogsPath();

			if (!std::filesystem::exists(log_folder))
				std::filesystem::create_directory(log_folder);

			stream_.open(log_folder / GenerateFilename(), std::ios::app);
		}

		std::string GenerateFilename()
		{	
			std::time_t t = std::chrono::system_clock::to_time_t(start_time_);
			std::tm tm;
#if defined(DL_PLATFORM_WINDOWS)
			localtime_s(&tm, &t);
#else
			localtime_r(&t, &tm);
#endif
			std::ostringstream oss;
			oss << "error+" << std::put_time(&tm, "%Y_%m_%d+%H_%M_%S") << ".txt";
			return oss.str();
		}

		std::ofstream stream_;
		std::mutex mutex_;
		std::chrono::system_clock::time_point start_time_;
	};
}

void Log::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_impl = spdlog::stdout_color_mt("ENGINE");
	s_impl->set_level(spdlog::level::trace);

	EnableAllCategories();
}

void Log::Shutdown()
{
	if (!LogFile::Inst().IsEmpty())
	{
		LogFile::Inst().Write("Application closed without crash", Log::Categories::Application);
		LogFile::Inst().Flush();
	}
}

void Log::SaveLogs(const std::string& exit_msg)
{
	LogFile::Inst().Write(exit_msg, Log::Categories::Application);
	LogFile::Inst().Flush();
}

void Log::ToggleCategory(Categories category, bool enable)
{
	if (enable)
		s_allowed_categories |= category;
	else
		s_allowed_categories &= category;
}

void Log::TraceImpl(const std::string& msg)
{
	s_impl->trace(msg);
}

void Log::InfoImpl(const std::string& msg)
{
	s_impl->info(msg);
}

void Log::WarnImpl(const std::string& msg)
{
	s_impl->warn(msg);
}

void Log::ErrorImpl(const std::string& msg, Log::Categories category)
{
	LogFile::Inst().Write(msg, category);
	s_impl->error(msg);
}

void Log::CriticalImpl(const std::string& msg, Log::Categories category)
{
	LogFile::Inst().Write(msg, category);
	s_impl->critical(msg);
}

void Log::EnableAllCategories()
{
	EnableCategory(Categories::EngineCore);
	EnableCategory(Categories::ECS);
	EnableCategory(Categories::Events);	
	EnableCategory(Categories::Renderer);
	EnableCategory(Categories::RendererAPI);
	EnableCategory(Categories::Platform);
	EnableCategory(Categories::Application);
}