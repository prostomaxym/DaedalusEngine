#include "dlpch.h"

#include "ConsoleWindow.h"

#include "Events/EventDispatcher.h"
#include "Events/ApplicationEvent.h"
#include "Config/KeybindConfig.h"

using namespace Daedalus;

namespace
{
    std::vector<std::string> commands = { "clear", "quit" };
}

void ConsoleWindow::OnEvent(Event& evt)
{
    EventDispatcher::ProcessEvent<KeyReleasedEvent>(evt, DL_BIND_EVENT_FN(ConsoleWindow::OnKeyReleased));
}

bool ConsoleWindow::OnKeyReleased(KeyReleasedEvent& event)
{
    const auto key_code = event.GetKeyCode();
    if (IsDevBuild() && key_code == KeybindConfig::GetKeyboardBind("Console"))
    {
        Toggle();
        auto& wnd = Application::GetInstance()->GetWindow();
        wnd.FreeCursor(IsShown());

        return true; 
    }

    return false;
}

void ConsoleWindow::Update(DeltaTime dt)
{
    static bool open = false;

    ImGui::Begin("Console", &open, ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowSize(ImVec2(1000, 800), ImGuiCond_Once);

    ImGui::BeginChild("ScrollingRegion", ImVec2(980, 780), 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiChildFlags_Borders);
    for (const auto& item : m_items)
        ImGui::TextUnformatted(item.c_str());

    if (m_scroll_to_bottom)
        ImGui::SetScrollHereY(1.0f);

    m_scroll_to_bottom = false;
    ImGui::EndChild();

    bool reclaim_focus = false;
    if (ImGui::InputText("Input", m_input_buffer, sizeof(m_input_buffer),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion,
        &TextEditCallbackStub, this))
    {
        ExecuteCommand(m_input_buffer);
        strcpy(m_input_buffer, "");
        reclaim_focus = true;
    }

    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1);

    ImGui::End();
}

void ConsoleWindow::Init()
{
    Clear();
}

void ConsoleWindow::Clear()
{
    m_items.clear();
    m_history.clear();
    m_input_buffer[0] = 0;
    m_history_pos = -1;
}

void ConsoleWindow::AddLog(const std::string& msg)
{
    m_items.push_back(msg);
}

void ConsoleWindow::ExecuteCommand(const char* command)
{
    AddLog(std::string("> ") + command);

    // Missing line – this adds input to history
    if (m_history.empty() || m_history.back() != command)
        m_history.push_back(command);

    m_history_pos = -1;

    ProcessCommand(command);

    m_scroll_to_bottom = true;
}

void ConsoleWindow::ProcessCommand(const char* command)
{
    if (strcmp(command, commands[0].c_str()) == 0)
    {
        Clear();
    }   
    else if (strcmp(command, commands[1].c_str()) == 0)
    {
        WindowCloseEvent evt;
        EventDispatcher::Dispatch(evt);
    }
    else
    {
        AddLog("Unknown command");
    }      
}

int ConsoleWindow::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
    auto* console = static_cast<ConsoleWindow*>(data->UserData);
    return console->TextEditCallback(data);
}

int ConsoleWindow::TextEditCallback(ImGuiInputTextCallbackData* data)
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
        const char* word_end = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf && !isspace(word_start[-1]))
            --word_start;

        std::string current_word(word_start, word_end);

        std::vector<std::string> candidates;
        for (const auto& cmd : commands)
        {
            if (cmd.compare(0, current_word.size(), current_word) == 0)
                candidates.push_back(cmd);
        }

        if (candidates.empty())
        {
            AddLog("No match for completion");
        }
        else if (candidates.size() == 1)
        {
            const std::string& completion = candidates[0];
            const size_t new_len = word_start - data->Buf + completion.length();

            if (new_len < static_cast<size_t>(data->BufSize))
            {
                std::memmove(const_cast<char*>(word_start), completion.c_str(), completion.size());
                const_cast<char*>(word_start)[completion.size()] = '\0';
                data->CursorPos = static_cast<int>(word_start - data->Buf + completion.size());
                data->BufTextLen = static_cast<int>(std::strlen(data->Buf));
                data->BufDirty = true;
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