#pragma once

#include <string>

#include "Utils/RingVector.h"
#include "Config/CVarRegistry.h"

#include "ImGuiWindow.h"
#include "Macros.h"

struct ImGuiInputTextCallbackData;

namespace Daedalus
{
    class KeyReleasedEvent;

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
        void SetTyping(bool typing);
        void UpdateInlineHint(const char* input, int cursor, bool has_selection);

        CVarRegistry m_cvars;
        std::string m_completion_input;
        std::string m_inline_hint;
        int m_completion_cursor = -1;
        bool m_completion_has_selection = false;
        RingVector<std::string, 50> m_items;
        RingVector<std::string, 50> m_history;
        int m_history_pos = -1;
        char m_input_buffer[256] = {};
        bool m_scroll_to_bottom = false;
        bool m_typing = false;
        bool m_keyboard_was_enabled = true;
    };
}