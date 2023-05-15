#pragma once

#include "Application.h"
#include "Daedalus/Utils/WorkingDirectory.h"

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	Daedalus::Log::Init();
	Daedalus::Log::Write(Daedalus::Log::Levels::Info, Daedalus::Log::Categories::EngineCore, "Initialized core logger;");

	Daedalus::WorkingDirectory::SetRootDirectory();
	auto app = Daedalus::CreateApplication();
	app->Run();
	delete app;

	return 0;
}