#include "SandboxApp.h"

#include "ExampleLayer.h"

using namespace Daedalus;

Application* ::CreateApplication()
{
	// Entry point is defined in engine
	return Sandbox::GetInstance();
}

Sandbox::Sandbox()
{
	Log::DisableCategory(Log::Categories::Events);
	PushLayer(new ExampleLayer());
}