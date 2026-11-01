#include "dlpch.h"

#include "ConsoleOverlay.h"
#include "ConsoleCompletion.h"
#include "ConsoleCommands.h"
#include "InputCommands.h"
#include "BuildInfo.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "Events/KeyEvent.h"

#include "Events/EventDispatcher.h"
#include "Events/ApplicationEvent.h"
#include "Config/KeybindConfig.h"
#include <sstream>

using namespace Daedalus;

void ConsoleOverlay::OnEvent(Event& evt)
{
    EventDispatcher::ProcessEvent<KeyReleasedEvent>(evt, DL_BIND_EVENT_FN(ConsoleOverlay::OnKeyReleased));
}

bool ConsoleOverlay::OnKeyReleased(KeyReleasedEvent& event)
{
    const auto key_code = event.GetKeyCode();
    if (IsDevBuild() && key_code == KeybindConfig::GetKeyboardBind("Console"))
    {
        Toggle();
        if (!IsShown())
            SetTyping(false);
        auto& wnd = Application::GetInstance()->GetWindow();
        wnd.FreeCursor(IsShown());

        return true; 
    }

    return false;
}

void ConsoleOverlay::Update(DeltaTime dt)
{
    bool open = true;

    ImGui::SetNextWindowSize(ImVec2(1000, 800), ImGuiCond_Once);
    const bool visible = ImGui::Begin("Console", &open, ImGuiWindowFlags_NoSavedSettings);
    if (!visible || !open)
    {
        ImGui::End();
        SetTyping(false);
        if (!open)
        {
            Show(false);
            Application::GetInstance()->GetWindow().FreeCursor(false);
        }
        return;
    }

    // Reserve the input row so only the output region needs to scroll.
    const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0.0f, -footer_height), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& item : m_items)
        ImGui::TextUnformatted(item.c_str());

    if (m_scroll_to_bottom)
        ImGui::SetScrollHereY(1.0f);

    m_scroll_to_bottom = false;
    ImGui::EndChild();

    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();

    bool reclaim_focus = false;
    const float input_width = ImGui::CalcItemWidth();
    const auto input_id = ImGui::GetID("Input");
    if (ImGui::InputText("Input", m_input_buffer, sizeof(m_input_buffer),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion,
        &TextEditCallbackStub, this))
    {
        ExecuteCommand(m_input_buffer);
        strcpy(m_input_buffer, "");
        reclaim_focus = true;
    }
    const bool input_active = ImGui::IsItemActive();
    SetTyping(input_active || reclaim_focus);

    if (input_active && !reclaim_focus)
    {
        if (const auto* state = ImGui::GetInputTextState(input_id))
        {
            UpdateInlineHint(m_input_buffer, state->GetCursorPos(), state->HasSelection());
            if (!m_inline_hint.empty())
            {
                const auto minimum = ImGui::GetItemRectMin();
                const auto maximum = ImGui::GetItemRectMax();
                const auto padding = ImGui::GetStyle().FramePadding;
                const float text_width = ImGui::CalcTextSize(m_input_buffer).x;
                auto* draw_list = ImGui::GetWindowDrawList();
                draw_list->PushClipRect(ImVec2(minimum.x + padding.x, minimum.y),
                    ImVec2(minimum.x + input_width - padding.x, maximum.y), true);
                draw_list->AddText(ImVec2(minimum.x + padding.x + text_width - state->Scroll.x, minimum.y + padding.y),
                    ImGui::GetColorU32(ImGuiCol_TextDisabled), m_inline_hint.c_str());
                draw_list->PopClipRect();
            }
        }
    }
    else
    {
        m_inline_hint.clear();
        m_completion_cursor = -1;
    }

    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1);

    ImGui::End();
}

void ConsoleOverlay::SetTyping(bool typing)
{
    if (typing == m_typing)
        return;

    m_typing = typing;
    if (typing)
    {
        m_keyboard_was_enabled = Input::IsKeyboardInputEnabled();
        Input::EnableKeyboardInput(false);
    }
    else
    {
        Input::EnableKeyboardInput(m_keyboard_was_enabled);
    }
}

void ConsoleOverlay::Init()
{
    Clear();
    m_cvars = CVarRegistry{};
    RegisterConfigCVars(m_cvars);
}

void ConsoleOverlay::Clear()
{
    m_items.clear();
    m_history.clear();
    m_input_buffer[0] = 0;
    m_history_pos = -1;
    m_completion_input.clear();
    m_inline_hint.clear();
    m_completion_cursor = -1;
}

void ConsoleOverlay::UpdateInlineHint(const char* input, int cursor, bool has_selection)
{
    const auto revision = KeybindConfig::GetRevision();
    if (m_completion_input == input && m_completion_cursor == cursor && m_completion_has_selection == has_selection && m_completion_revision == revision)
        return;
    m_completion_revision = revision;
    m_completion_input = input;
    m_completion_cursor = cursor;
    m_completion_has_selection = has_selection;
    m_inline_hint = ConsoleCompletion::Hint(m_completion_input, static_cast<size_t>(cursor), has_selection, sizeof(m_input_buffer), m_cvars);
}

void ConsoleOverlay::AddLog(const std::string& msg)
{
    m_items.push_back(msg);
}

void ConsoleOverlay::ExecuteCommand(const char* command)
{
    AddLog(std::string("> ") + command);

    if (m_history.empty() || m_history.back() != command)
        m_history.push_back(command);

    m_history_pos = -1;

    ProcessCommand(command);

    m_scroll_to_bottom = true;
}

void ConsoleOverlay::ProcessCommand(const char* command)
{
    std::vector<std::string> output;
    if (ConsoleCommands::ExecuteHelp(command, output) || InputCommands::Execute(command, output) || m_cvars.Execute(command, output))
    {
        for (const auto& line : output)
            AddLog(line);
        return;
    }
    std::istringstream input(command);
    std::string verb, extra;
    input >> verb >> extra;
    if (verb == "clear" && extra.empty())
    {
        Clear();
    }   
    else if (verb == "quit" && extra.empty())
    {
        WindowCloseEvent evt;
        EventDispatcher::Dispatch(evt);
    }
    else if (verb == "version" && extra.empty())
    {
        AddLog(GetBuildInfo());
    }
    else
    {
        AddLog("Unknown command");
    }      
}

int ConsoleOverlay::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
    auto* console = static_cast<ConsoleOverlay*>(data->UserData);
    return console->TextEditCallback(data);
}

int ConsoleOverlay::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackHistory:
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (m_history_pos == -1)
                m_history_pos = static_cast<int>(m_history.size()) - 1;
            else if (m_history_pos > 0)
                --m_history_pos;
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (m_history_pos != -1 && ++m_history_pos >= static_cast<int>(m_history.size()))
                m_history_pos = -1;
        }

        if (m_history_pos >= 0)
        {
            const std::string& history_cmd = m_history[m_history_pos];
            std::snprintf(data->Buf, data->BufSize, "%s", history_cmd.c_str());
            data->BufDirty = true;
            data->BufTextLen = static_cast<int>(history_cmd.size());
            data->CursorPos = data->BufTextLen;
            data->SelectionStart = data->SelectionEnd = data->CursorPos;
        }
        break;

    case ImGuiInputTextFlags_CallbackCompletion:
    {
        UpdateInlineHint(data->Buf, data->CursorPos, data->SelectionStart != data->SelectionEnd);
        if (!m_inline_hint.empty())
        {
            data->InsertChars(data->CursorPos, m_inline_hint.c_str());
            data->SelectionStart = data->SelectionEnd = data->CursorPos;
            break;
        }
        const auto matches = ConsoleCompletion::Find(std::string_view(data->Buf, static_cast<size_t>(data->BufTextLen)),
            static_cast<size_t>(data->CursorPos), m_cvars);
        const auto& candidates = matches.candidates;
        if (candidates.empty())
        {
            AddLog("No match for completion");
        }
        else if (candidates.size() == 1)
        {
            const std::string& completion = candidates[0];
            const size_t new_len = static_cast<size_t>(data->BufTextLen) - matches.length + completion.size();

            if (new_len < static_cast<size_t>(data->BufSize))
            {
                const int offset = static_cast<int>(matches.offset);
                data->DeleteChars(offset, static_cast<int>(matches.length));
                data->InsertChars(offset, completion.c_str());
                data->CursorPos = offset + static_cast<int>(completion.size());
                data->SelectionStart = data->SelectionEnd = data->CursorPos;
            }
        }
        else
        {
            std::string msg = "Possible completions:";
            for (const auto& candidate : candidates)
                msg += " " + candidate;
            AddLog(msg);
        }

        break;
    }
    }

    return 0;
}
