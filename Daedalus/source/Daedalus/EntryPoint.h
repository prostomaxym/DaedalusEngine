#pragma once

#ifdef DL_PLATFORM_WINDOWS

extern Daedalus::Application* Daedalus::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Daedalus::CreateApplication();
	app->Run();
	delete app;

	return 0;
}
#endif
