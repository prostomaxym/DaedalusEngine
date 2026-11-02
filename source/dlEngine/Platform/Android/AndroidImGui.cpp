#include "Platform/ImGuiBackend.h"
#include "Core/Window.h"
#include "AndroidInput.h"
#include <android/native_window.h>
#include <imgui.h>
#include <imgui_impl_android.h>

namespace Daedalus::ImGuiBackend
{
	void Initialize(Window& window)
	{
		ImGui_ImplAndroid_Init(static_cast<ANativeWindow*>(window.GetNativeWindow()));
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	}

	void Shutdown() { ImGui_ImplAndroid_Shutdown(); }
	void NewFrame() { ImGui_ImplAndroid_NewFrame(); }
	void DrawControls() { AndroidInput::DrawControls(); }
	void RenderPlatformWindows() {}
	// ImGui selects its precision-qualified GLES shader sources for version 300 specifically.
	const char* ShaderVersion() { return "#version 300 es"; }
}
