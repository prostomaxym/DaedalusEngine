#include "dlpch.h"
#include "DebugOverlay.h"
#include "Tabs/GeneralDebugTab.h"
#include "Tabs/RenderingDebugTab.h"
#include "Tabs/ResourcesDebugTab.h"
#include "Tabs/ProfilerDebugTab.h"
#include "Tabs/LoggingDebugTab.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Config/KeybindConfig.h"
#include "Events/KeyEvent.h"
#include "Events/EventDispatcher.h"
#include "Renderer/API/Renderer.h"
#include <imgui.h>

using namespace Daedalus;

DebugOverlay::DebugOverlay() = default;
DebugOverlay::~DebugOverlay() = default;

void DebugOverlay::Init()
{
	m_general_tab = std::make_unique<GeneralDebugTab>();
	m_rendering_tab = std::make_unique<RenderingDebugTab>();
	m_resources_tab = std::make_unique<ResourcesDebugTab>();
	m_profiler_tab = std::make_unique<ProfilerDebugTab>();
	m_logging_tab = std::make_unique<LoggingDebugTab>();
	m_general_tab->Init();
	m_profiler_tab->Init();

	// All sizes are suitable for QHD, scale for big for FHD to fit everything on screen
	// Maybe will implement scalability of debug layer later, right now just scale
	const auto [w, h] = Renderer::GetResolution();
	if (w <= 1920)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(1.0 / 1.33f);
	}
}

void DebugOverlay::Update(DeltaTime dt)
{
	bool open = true;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	// The log list has no natural size, so auto-resize would keep the previous tab's size and clip it.
	if (m_logs_tab_active)
		ImGui::SetNextWindowSize(ImVec2(1200.f, 800.f), ImGuiCond_Always);
	else
		window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("Debug Overlay", &open, window_flags);

	bool logs_tab_active = false;
	if (ImGui::BeginTabBar("Debug Overlay", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("General"))
		{
			m_general_tab->Render(dt);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Rendering"))
		{
			m_rendering_tab->Render();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Resources"))
		{
			m_resources_tab->Render();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Profiler"))
		{
			m_profiler_tab->Render();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Logging"))
		{
			logs_tab_active = true;
			m_logging_tab->Render();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
	m_logs_tab_active = logs_tab_active;
	if (!open)
	{
		Show(false);
		Application::GetInstance()->GetWindow().FreeCursor(false);
	}
}

void DebugOverlay::OnEvent(Event& evt)
{
	EventDispatcher::ProcessEvent<KeyReleasedEvent>(evt, DL_BIND_EVENT_FN(DebugOverlay::OnKeyReleased));
}

bool DebugOverlay::OnKeyReleased(KeyReleasedEvent& event)
{
	const auto key_code = event.GetKeyCode();
	if (IsDevBuild() && key_code == KeybindConfig::GetKeyboardBind("Debug"))
	{
		Toggle();
		auto& wnd = Application::GetInstance()->GetWindow();
		wnd.FreeCursor(IsShown());
		return true;
	}
	return false;
}