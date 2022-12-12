#include <Daedalus.h>

class ExampleLayer : public Daedalus::Layer
{
public:
	ExampleLayer() : Layer("Example") {}

	void OnUpdate() override
	{
		//DL_INFO("ExampleLayer::Update");
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
	}
};

Daedalus::Application* Daedalus::CreateApplication()
{
	// Entry point is defined in engine
	return Sandbox::GetInstance();
}