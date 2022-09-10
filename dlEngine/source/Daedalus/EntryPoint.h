#pragma once

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	Daedalus::Log::Init();
	DL_CORE_WARN("Initialized core logger;");
	DL_INFO("Initialized client logger;");

	auto app = Daedalus::CreateApplication();
	app->Run();
	delete app;

	return 0;
}