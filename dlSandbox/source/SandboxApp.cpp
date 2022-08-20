#include <Daedalus.h>

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