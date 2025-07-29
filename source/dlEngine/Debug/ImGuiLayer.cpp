#include "dlpch.h"
#include "ImGuiLayer.h"

#include <GLFW/glfw3.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>
#include <implot.h>
#include <implot3d.h>

#include "Macros.h"
#include "Core/Application.h"
#include "Events/EventDispatcher.h"
#include "Utils/WorkingDirectory.h"
#include "Config/PathConfig.h"

using namespace Daedalus;

namespace
{
	bool WantToLoadConfig = false;
}

ImGuiLayer::ImGuiLayer()
	: Layer("ImGuiLayer")
{
}

void ImGuiLayer::OnAttach()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImPlot3D::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.IniFilename = nullptr;

	if (WantToLoadConfig)
		DoLoadConfig();

	float fontSize = 18.0f;// *2.0f;

	const auto font_filename = (WorkingDirectory::GetAssetsDirectory() / "fonts/calibrib.ttf").string();

	io.Fonts->AddFontFromFileTTF(font_filename.c_str(), fontSize);
	io.FontDefault = io.Fonts->AddFontFromFileTTF(font_filename.c_str(), fontSize);

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	SetDarkThemeColors();

	const auto app = Application::GetInstance();
	GLFWwindow* window = static_cast<GLFWwindow*>(app->GetWindow().GetNativeWindow());

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	InitWindow();

	Log::Write(Log::Levels::Info, Log::Categories::EngineCore, "ImGui Layer is attached;");
}

void ImGuiLayer::OnDetach()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	ImPlot3D::DestroyContext();

	Log::Write(Log::Levels::Warn, Log::Categories::EngineCore, "ImGui Layer is detached;");
}

void ImGuiLayer::OnEvent(Event& evt)
{
	if (m_block_events)
	{
		ImGuiIO& io = ImGui::GetIO();
		evt.AddHandle(evt.IsInCategory(EventCategory::MouseEvent) && io.WantCaptureMouse);
		evt.AddHandle(evt.IsInCategory(EventCategory::KeyboardEvent) && io.WantCaptureKeyboard);
	}
}

void ImGuiLayer::OnUpdate(DeltaTime dt)
{
	if (!m_show)
		return;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	
	Update(dt);

	ImGuiIO& io = ImGui::GetIO();
	const auto app = Application::GetInstance();
	io.DisplaySize = ImVec2(static_cast<float>(app->GetWindow().GetWidth()), static_cast<float>(app->GetWindow().GetHeight()));

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void ImGuiLayer::SetDarkThemeColors()
{
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

void ImGuiLayer::LoadConfig()
{
	WantToLoadConfig = true; // Postpone loading until contex is created
}

void ImGuiLayer::SaveConfig()
{
	auto ini_path = WorkingDirectory::GetRootDirectory() / PathConfig::GetConfigPath() / "imgui.ini";
	ImGui::SaveIniSettingsToDisk(ini_path.string().c_str());
}

void ImGuiLayer::DoLoadConfig()
{
	auto ini_path = PathConfig::GetConfigPath() / "imgui.ini";
	if (std::filesystem::exists(ini_path))
		ImGui::LoadIniSettingsFromDisk(ini_path.string().c_str());
}