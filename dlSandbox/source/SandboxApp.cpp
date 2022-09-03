#include <Daedalus.h>

//Force higher performance GPU usage
extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

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