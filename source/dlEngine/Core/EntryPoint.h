#pragma once

#include "Application.h"
#include "Utils/WorkingDirectory.h"

#include <memory>

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	Daedalus::Log::Init();
	Daedalus::Log::Write(Daedalus::Log::Levels::Info, Daedalus::Log::Categories::EngineCore, "Initialized core logger;");

	Daedalus::WorkingDirectory::SetRootDirectory();
	{
		const std::unique_ptr<Daedalus::Application> app(Daedalus::CreateApplication());
		app->Run();
	}

	Daedalus::Log::Shutdown();

	return 0;
}