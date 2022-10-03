#include "dlpch.h"
#include "ImGuiLayer.h"

#include "Daedalus/Core/Application.h"
#include "Daedalus/Core/Core.h"
#include "Daedalus/Events/EventDispatcher.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

using namespace Daedalus;

ImGuiLayer::ImGuiLayer() :
	Layer("ImGui Layer"),
	m_time(0.0)
{
}

ImGuiLayer::~ImGuiLayer()
{
}

void ImGuiLayer::OnAttach()
{
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

	ImGui_ImplOpenGL3_Init("#version 450");
}

void ImGuiLayer::OnDetach()
{
}

void ImGuiLayer::OnUpdate()
{
	ImGuiIO& io = ImGui::GetIO();

	Application* app = Application::GetInstance();
	io.DisplaySize = ImVec2(app->GetWindow().GetWidth(), app->GetWindow().GetHeight());

	const auto time = glfwGetTime();
	io.DeltaTime = m_time > 0.0 ? (time - m_time) : (1.0 / 60.);
	m_time = time;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	static bool show = true;
	ImGui::ShowDemoWindow(&show);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);

	switch (event.GetEventType())
	{
	case EventType::MouseButtonPressed:
		dispatcher.Dispatch<MouseButtonPressedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
		break;
	case EventType::MouseButtonReleased:
		dispatcher.Dispatch<MouseButtonReleasedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
		break;
	case EventType::MouseMoved:
		dispatcher.Dispatch<MouseMovedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
		break;
	case EventType::MouseScrolled:
		dispatcher.Dispatch<MouseScrolledEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnMouseScrolledEvent));
		break;
	case EventType::KeyPressed:
		dispatcher.Dispatch<KeyPressedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
		break;
	case EventType::KeyReleased:
		dispatcher.Dispatch<KeyReleasedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
		break;
	case EventType::KeyTyped:
		dispatcher.Dispatch<KeyTypedEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
		break;
	case EventType::WindowResize:
		dispatcher.Dispatch<WindowResizeEvent>(DL_BIND_EVENT_FN(ImGuiLayer::OnWindowResizedEvent));
		break;
	default:
		break;
	}
}

bool Daedalus::ImGuiLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent& event)
{
	ImGuiIO & io= ImGui::GetIO();
	io.AddMouseButtonEvent(event.GetMouseButton(), true);

	return false;
}

bool Daedalus::ImGuiLayer::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(event.GetMouseButton(), false);

	return false;
}

bool Daedalus::ImGuiLayer::OnMouseMovedEvent(MouseMovedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMousePosEvent(event.GetX(), event.GetY());

	return false;
}

bool Daedalus::ImGuiLayer::OnMouseScrolledEvent(MouseScrolledEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseWheelEvent(event.GetOffsetX(), event.GetOffsetY());

	return false;
}

bool Daedalus::ImGuiLayer::OnKeyPressedEvent(KeyPressedEvent& event)
{
	return false;
}

bool Daedalus::ImGuiLayer::OnKeyReleasedEvent(KeyReleasedEvent& event)
{
	return false;
}

bool Daedalus::ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	int keycode = event.GetKeyCode();

	if (keycode > 0 && keycode < 0x10000)
		io.AddInputCharacter(keycode);

	return false;
}

bool Daedalus::ImGuiLayer::OnWindowResizedEvent(WindowResizeEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(event.GetWidth(), event.GetHeight());
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	glViewport(0.f, 0.f, event.GetWidth(), event.GetHeight());

	return false;
}