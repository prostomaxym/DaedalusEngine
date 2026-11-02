#include "Platform/ImGuiBackend.h"
#include "Core/Window.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

namespace Daedalus::ImGuiBackend
{
	void Initialize(Window& window)
	{
		ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window.GetNativeWindow()), true);
	}

	void Shutdown() { ImGui_ImplGlfw_Shutdown(); }
	void NewFrame() { ImGui_ImplGlfw_NewFrame(); }
	void DrawControls() {}
	const char* ShaderVersion() { return "#version 450"; }

	void RenderPlatformWindows()
	{
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			auto* previous_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(previous_context);
		}
	}
}
