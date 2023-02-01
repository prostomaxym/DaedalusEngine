#pragma once

#include "Application.h"
#include "Daedalus/Utils/WorkingDirectory.h"

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	Daedalus::Log::Init();
	DL_CORE_WARN("Initialized core logger;");
	DL_INFO("Initialized client logger;");

	Daedalus::WorkingDirectory::SetRootDirectory();
	auto app = Daedalus::CreateApplication();
	app->Run();
	delete app;

	return 0;
}