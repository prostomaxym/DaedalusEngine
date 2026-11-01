#include "dlpch.h"
#include "LoggingDebugTab.h"
#include <imgui.h>
#include "Platform/Platform.h"

using namespace Daedalus;

namespace
{
	ImVec4 GetColorForLevel(Log::Levels level)
	{
		switch (level)
		{
		case Log::Levels::Trace:    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
		case Log::Levels::Info:     return ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
		case Log::Levels::Warn:     return ImVec4(0.7f, 0.7f, 0.0f, 1.0f);
		case Log::Levels::Error:    return ImVec4(0.7f, 0.0f, 0.0f, 1.0f);
		case Log::Levels::Critical: return ImVec4(0.7f, 0.0f, 0.7f, 1.0f);
		default:                   return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

void LoggingDebugTab::Render()
{
	m_log_selection_panel.Draw();
	RenderLogsPage();
}

void LoggingDebugTab::RenderLogsPage()
{
	//static bool open = false;
	ImGui::BeginChild("Log Viewer", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar);

	const auto& entries = Log::Storage::GetEntries();
	for (const auto& entry : entries)
	{
		if (!m_log_selection_panel.IsCategoryEnabled(entry.category)
			|| !m_log_selection_panel.IsLevelEnabled(entry.level))
			continue;

		ImGui::TextColored(GetColorForLevel(entry.level), "[%s][%s][%s]: %s",
			Platform::FormatTimestamp(entry.timestamp).c_str(),
			Log::ToString(entry.level),
			Log::ToString(entry.category),
			entry.message.c_str());
	}

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f); // auto-scroll to bottom

	ImGui::EndChild();
}

void LoggingDebugTab::LogFilterPanel::Draw()
{
	if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_Framed))
	{
		ImGui::BeginChild("Cat:", { 200.f, 220.f }, 0, ImGuiChildFlags_Borders);
		ImGui::Text("Categories:");
		DrawCategoryCheckbox(Log::Categories::EngineCore);
		DrawCategoryCheckbox(Log::Categories::ECS);
		DrawCategoryCheckbox(Log::Categories::Renderer);
		DrawCategoryCheckbox(Log::Categories::RendererAPI);
		DrawCategoryCheckbox(Log::Categories::Events);
		DrawCategoryCheckbox(Log::Categories::Platform);
		DrawCategoryCheckbox(Log::Categories::Application);
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Lev:", { 200.f, 220.f }, 0, ImGuiChildFlags_Borders);
		ImGui::Text("Levels:");
		DrawLevelCheckbox(Log::Levels::Trace);
		DrawLevelCheckbox(Log::Levels::Info);
		DrawLevelCheckbox(Log::Levels::Warn);
		DrawLevelCheckbox(Log::Levels::Error);
		DrawLevelCheckbox(Log::Levels::Critical);
		ImGui::Separator();
		ImGui::Checkbox("Stop updating", &Log::GetLockUpdating());
		ImGui::EndChild();
		ImGui::Separator();
	}
}

LoggingDebugTab::LogFilterPanel::LogFilterPanel()
{
	std::fill(level_flags_.begin(), level_flags_.end(), true);
}

bool LoggingDebugTab::LogFilterPanel::IsLevelEnabled(Log::Levels level) const
{
	return level_flags_[static_cast<int>(level)];
}

bool LoggingDebugTab::LogFilterPanel::IsCategoryEnabled(Log::Categories category_bit) const
{
	return (Log::GetAllowedCategories() & category_bit) != 0;
}

void LoggingDebugTab::LogFilterPanel::DrawCategoryCheckbox(Log::Categories cat)
{
	const int bit = static_cast<int>(cat);
	bool enabled = (Log::GetAllowedCategories() & bit) != 0;
	if (ImGui::Checkbox(Log::ToString(cat), &enabled))
	{
		if (enabled)
			Log::EnableCategory(cat);
		else
			Log::DisableCategory(cat);
	}
}

void LoggingDebugTab::LogFilterPanel::DrawLevelCheckbox(Log::Levels level)
{
	int i = static_cast<int>(level);
	ImGui::Checkbox(Log::ToString(level), &level_flags_[i]);
}

