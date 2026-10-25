#pragma once

#include <vector>
#include <imgui.h>

#include "Utils/RingVector.h"

#include "ImGuiWindow.h"
#include "Events/KeyEvent.h"

namespace Daedalus
{
    class DAEDALUS_API ConsoleOverlay : public ImGuiWindow
    {
    public:
        ConsoleOverlay() = default;
        virtual ~ConsoleOverlay() = default;

        void Init() override;
        void Update(DeltaTime dt) override;

        void OnEvent(Event& evt) override;
        bool OnKeyReleased(KeyReleasedEvent& event);

    private:
        static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

        void Clear();

        void AddLog(const std::string& msg);    
        void ExecuteCommand(const char* command);
        void ProcessCommand(const char* command);
        int TextEditCallback(ImGuiInputTextCallbackData* data);

        RingVector<std::string, 50> m_items;
        RingVector<std::string, 50> m_history;
        int m_history_pos = -1;
        char m_input_buffer[256] = {};
        bool m_scroll_to_bottom = false;
    };
}