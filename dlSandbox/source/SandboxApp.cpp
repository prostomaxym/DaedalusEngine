#include <Daedalus.h>

//Force higher performance GPU usage
#ifdef DL_PLATFORM_WINDOWS
extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

class ExampleLayer : public Daedalus::Layer
{
public:
	ExampleLayer() : Layer("Example") {}

	void OnUpdate() override
	{
		DL_INFO("ExampleLayer::Update");
	}

	void OnEvent(Daedalus::Event& event) override
	{
		DL_TRACE("{0}", event);
	}
};

class Sandbox : public Daedalus::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}
};

Daedalus::Application* Daedalus::CreateApplication()
{
	return new Sandbox();
}