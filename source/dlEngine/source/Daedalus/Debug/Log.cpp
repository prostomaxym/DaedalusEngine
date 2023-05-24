#include "dlpch.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

using namespace Daedalus;

std::shared_ptr<spdlog::logger> Log::s_core_logger;
int Log::s_allowed_categories = 0;

void Log::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_core_logger = spdlog::stdout_color_mt("ENGINE");
	s_core_logger->set_level(spdlog::level::trace);

	EnableAllCategories();
}

void Log::ToggleCategory(Categories category, bool enable)
{
	if (enable)
		s_allowed_categories |= category;
	else
		s_allowed_categories &= category;
}

void Log::EnableAllCategories()
{
	EnableCategory(Categories::EngineCore);
	EnableCategory(Categories::ECS);
	EnableCategory(Categories::Events);	
	EnableCategory(Categories::Renderer);
	EnableCategory(Categories::Platform);
}
