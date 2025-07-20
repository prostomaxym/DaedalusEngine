#include "SandboxApp.h"

#include "ExampleLayer.h"
#include "Config/KeybindConfig.h"
using namespace Daedalus;

Application* Daedalus::CreateApplication()
{
	// Entry point is defined in engine
	return Sandbox::GetInstance();
}

Sandbox::Sandbox()
{
	Log::DisableCategory(Log::Categories::Events);
	PushLayer(std::make_unique<ExampleLayer>());

	if (KeybindConfig::IsEmpty())
	{
		KeybindConfig::SetKeyboardBind("MoveForward", DL_KEY_W);
		KeybindConfig::SetKeyboardBind("MoveBackward", DL_KEY_S);
		KeybindConfig::SetKeyboardBind("MoveLeft", DL_KEY_A);
		KeybindConfig::SetKeyboardBind("MoveRight", DL_KEY_D);
		KeybindConfig::SetKeyboardBind("MoveUp", DL_KEY_SPACE);
		KeybindConfig::SetKeyboardBind("MoveDown", DL_KEY_C);
		KeybindConfig::SetKeyboardBind("Flashlight", DL_KEY_F);
		KeybindConfig::SetKeyboardBind("ChangePOV", DL_KEY_V);
	}
}
