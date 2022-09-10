#include <Daedalus.h>

//Force higher performance GPU usage
#ifdef DL_PLATFORM_WINDOWS
extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

class Sandbox : public Daedalus::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}

};

Daedalus::Application* Daedalus::CreateApplication()
{
	return new Sandbox();
}