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
		//DL_INFO("ExampleLayer::Update");
		DL_TRACE(Daedalus::Input::IsKeyPressed(DL_KEY_TAB));
	}

	void OnEvent(Daedalus::Event& event) override
	{
		DL_TRACE("{0}", event);
	}
};

class Sandbox : public Daedalus::Application
{
public:
	static Application* GetInstance()
	{
		if (!s_instance)
		{
			s_instance = new Sandbox();
		}

		return s_instance;
	}

	~Sandbox()
	{

	}

private:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Daedalus::ImGuiLayer());
	}
};

Daedalus::Application* Daedalus::CreateApplication()
{
	// Entry point is defined in engine
	return Sandbox::GetInstance();
}