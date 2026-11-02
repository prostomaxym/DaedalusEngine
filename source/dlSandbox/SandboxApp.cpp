#include "SandboxApp.h"

#include "ExampleLayer.h"
#include "Config/KeybindConfig.h"
using namespace Daedalus;

Application* Daedalus::CreateApplication()
{
	// Entry point is defined in engine and takes ownership of the returned application
	return new Sandbox();
}

Sandbox::Sandbox()
{
	Log::DisableCategory(Log::Categories::Events);
	PushLayer(std::make_unique<ExampleLayer>());
}
