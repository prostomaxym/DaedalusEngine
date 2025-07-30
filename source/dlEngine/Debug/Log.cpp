#include "dlpch.h"
#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace Daedalus;

std::shared_ptr<spdlog::logger> s_impl = nullptr;
int Log::s_allowed_categories = 0;
bool Log::s_stop_updating = false;

void Log::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_impl = spdlog::stdout_color_mt("ENGINE");
	s_impl->set_level(spdlog::level::trace);

	EnableAllCategories();
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

void Log::ErrorImpl(const std::string& msg)
{
	s_impl->error(msg);
}

void Log::CriticalImpl(const std::string& msg)
{
	s_impl->critical(msg);
}

void Log::EnableAllCategories()
{
	EnableCategory(Categories::EngineCore);
	EnableCategory(Categories::ECS);
	EnableCategory(Categories::Events);	
	EnableCategory(Categories::Renderer);
	EnableCategory(Categories::Platform);
	EnableCategory(Categories::Application);
}