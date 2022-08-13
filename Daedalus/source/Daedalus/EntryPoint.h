#pragma once

#ifdef DL_PLATFORM_WINDOWS

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	Daedalus::Log::Init();
	DL_CORE_WARN("Initialized core logger;");
	int a = 5;
	DL_INFO("Initialized client logger;");
	DL_INFO("Test var = {0}", a);

	auto app = Daedalus::CreateApplication();
	app->Run();
	delete app;

	return 0;
}
#endif
