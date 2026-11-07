#ifdef DL_PLATFORM_ANDROID

#include "AndroidInput.h"
#include "AndroidRuntime.h"
#include "Config/KeybindConfig.h"
#include "Core/Application.h"
#include "Events/EventsEngine.h"
#include "Events/KeyEvent.h"
#include <android/native_window.h>
#include <imgui.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>

namespace Daedalus
{
	namespace
	{
		int TranslateKey(int code)
		{
			if (code >= AKEYCODE_A && code <= AKEYCODE_Z) return DL_KEY_A + code - AKEYCODE_A;
			if (code >= AKEYCODE_0 && code <= AKEYCODE_9) return DL_KEY_0 + code - AKEYCODE_0;
			switch (code)
			{
			case AKEYCODE_SPACE: return DL_KEY_SPACE;
			case AKEYCODE_ENTER: return DL_KEY_ENTER;
			case AKEYCODE_ESCAPE: return DL_KEY_ESCAPE;
			case AKEYCODE_TAB: return DL_KEY_TAB;
			case AKEYCODE_DEL: return DL_KEY_BACKSPACE;
			case AKEYCODE_SHIFT_LEFT: return DL_KEY_LEFT_SHIFT;
			case AKEYCODE_SHIFT_RIGHT: return DL_KEY_RIGHT_SHIFT;
			case AKEYCODE_CTRL_LEFT: return DL_KEY_LEFT_CONTROL;
			case AKEYCODE_CTRL_RIGHT: return DL_KEY_RIGHT_CONTROL;
			case AKEYCODE_DPAD_UP: return DL_KEY_UP;
			case AKEYCODE_DPAD_DOWN: return DL_KEY_DOWN;
			case AKEYCODE_DPAD_LEFT: return DL_KEY_LEFT;
			case AKEYCODE_DPAD_RIGHT: return DL_KEY_RIGHT;
			default: return -1;
			}
		}

		// Android starts without a keybind INI, and the generic game does not create defaults.
		// Touch controls emulate these keys, so missing actions get fallback bindings.
		void EnsureTouchActionBindings()
		{
			constexpr std::array<std::pair<const char*, int>, 8> defaults{ {
				{ "MoveForward", DL_KEY_W }, { "MoveBackward", DL_KEY_S }, { "MoveLeft", DL_KEY_A }, { "MoveRight", DL_KEY_D },
				{ "MoveUp", DL_KEY_SPACE }, { "MoveDown", DL_KEY_C }, { "Flashlight", DL_KEY_F }, { "ChangePOV", DL_KEY_V } } };
			for (const auto& [action, key] : defaults)
			{
				if (!KeybindConfig::FindActionBind(action))
					KeybindConfig::SetKeyboardBind(action, key);
			}
		}
	}

	AndroidInput::AndroidInput() { AndroidRuntime::Get().SetInput(this); }
	AndroidInput::~AndroidInput() { AndroidRuntime::Get().SetInput(nullptr); }
	void AndroidInput::Reset() { m_pending.fill(false); m_mouse_buttons.fill(false); m_touch.Reset(); m_toggle_pulses.fill(false); }
	void AndroidInput::UpdateImpl()
	{
		if (!m_bindings_ensured)
		{
			EnsureTouchActionBindings();
			m_bindings_ensured = true;
		}
		if (auto* window = AndroidRuntime::Get().GetApp().window)
			m_touch.Resize(static_cast<float>(ANativeWindow_getWidth(window)), static_cast<float>(ANativeWindow_getHeight(window)));
		m_previous = m_current;
		m_current = m_pending;
		const auto [x, y] = m_touch.Movement();
		const auto press = [this](const char* action, bool down)
		{
			const auto* binding = KeybindConfig::FindActionBind(action);
			if (down && binding && binding->keyboard >= 0 && binding->keyboard <= DL_KEY_MENU)
				m_current[binding->keyboard] = true;
		};
		press("MoveForward", y < -0.25f);
		press("MoveBackward", y > 0.25f);
		press("MoveLeft", x < -0.25f);
		press("MoveRight", x > 0.25f);
		press("MoveUp", m_touch.Up());
		press("MoveDown", m_touch.Held(TouchControls::Button::Down));
		const std::array<const char*, 2> toggle_actions{ "ChangePOV", "Flashlight" };
		const std::array<TouchControls::Button, 2> toggle_buttons{ TouchControls::Button::POV, TouchControls::Button::Flashlight };
		for (size_t i = 0; i < toggle_actions.size(); ++i)
		{
			const bool released = m_touch.ConsumeRelease(toggle_buttons[i]);
			if (m_toggle_pulses[i]) m_toggle_pulses[i] = false;
			else if (released) { press(toggle_actions[i], true); m_toggle_pulses[i] = true; }
		}
		if constexpr (IsDevBuild())
		{
			const auto toggle_overlay = [this](TouchControls::Button button, const char* action, int fallback)
			{
				if (!m_touch.ConsumeRelease(button)) return;
				if (!KeybindConfig::FindActionBind(action)) KeybindConfig::SetKeyboardBind(action, fallback);
				const int code = KeybindConfig::GetKeyboardBind(action);
				if (code >= 0) Application::GetInstance()->GetEventEngine().AddEvent(std::make_unique<KeyReleasedEvent>(code));
			};
			toggle_overlay(TouchControls::Button::Console, "Console", DL_KEY_GRAVE_ACCENT);
			toggle_overlay(TouchControls::Button::Debug, "Debug", DL_KEY_F3);
		}
		const auto [dx, dy] = m_touch.ConsumeLook();
		m_mouse.first += dx;
		m_mouse.second += dy;
	}

	void AndroidInput::DrawControls()
	{
		const auto& touch = static_cast<AndroidInput*>(GetInstance())->m_touch;
		const auto radius = touch.Radius();
		if (radius <= 0.f) return;
		auto* draw = ImGui::GetForegroundDrawList();
		const auto [cx, cy] = touch.MovementCenter();
		const auto [x, y] = touch.Movement();
		draw->AddCircleFilled({ cx, cy }, radius, IM_COL32(30, 30, 30, 90));
		draw->AddCircle({ cx, cy }, radius, IM_COL32(255, 255, 255, 150));
		draw->AddCircleFilled({ cx + x * radius, cy + y * radius }, radius * 0.35f, IM_COL32(255, 255, 255, 140));
		for (size_t i = 0; i < TouchControls::ButtonCount; ++i)
		{
			const auto button = static_cast<TouchControls::Button>(i);
			if (!IsDevBuild() && (button == TouchControls::Button::Console || button == TouchControls::Button::Debug)) continue;
			const auto [bx, by] = touch.ButtonCenter(button);
			const float button_radius = touch.ButtonRadius(button);
			draw->AddCircleFilled({ bx, by }, button_radius, touch.Held(button) ? IM_COL32(100, 160, 255, 200) : IM_COL32(30, 30, 30, 150));
			draw->AddCircle({ bx, by }, button_radius, IM_COL32(255, 255, 255, 180));
			const auto* label = TouchControls::ButtonLabels[i];
			const auto text = ImGui::CalcTextSize(label);
			draw->AddText({ bx - text.x * 0.5f, by - text.y * 0.5f }, IM_COL32(255, 255, 255, 240), label);
		}
	}
	bool AndroidInput::IsKeyPressedImpl(int code) { return code >= 0 && code <= DL_KEY_MENU && m_current[code] && !m_previous[code]; }
	bool AndroidInput::IsKeyHoldImpl(int code) { return code >= 0 && code <= DL_KEY_MENU && m_current[code] && m_previous[code]; }
	bool AndroidInput::IsKeyReleasedImpl(int code) { return code >= 0 && code <= DL_KEY_MENU && !m_current[code] && m_previous[code]; }
	bool AndroidInput::IsMouseButtonPressedImpl(int button) { return button >= 0 && button < 3 && m_mouse_buttons[button]; }

	void AndroidInput::HandleInput(AInputEvent* event)
	{
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
		{
			const auto code = AKeyEvent_getKeyCode(event);
			const auto action = AKeyEvent_getAction(event);
			if (code == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP)
				ANativeActivity_finish(AndroidRuntime::Get().GetApp().activity);
			const auto translated = TranslateKey(code);
			if (translated >= 0 && translated <= DL_KEY_MENU) m_pending[translated] = action != AKEY_EVENT_ACTION_UP;
		}
		else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
		{
			const auto raw_action = AMotionEvent_getAction(event);
			const auto action = raw_action & AMOTION_EVENT_ACTION_MASK;
			if ((AInputEvent_getSource(event) & AINPUT_SOURCE_TOUCHSCREEN) == AINPUT_SOURCE_TOUCHSCREEN)
			{
				if (auto* window = AndroidRuntime::Get().GetApp().window)
					m_touch.Resize(static_cast<float>(ANativeWindow_getWidth(window)), static_cast<float>(ANativeWindow_getHeight(window)));
				const auto index = static_cast<size_t>((raw_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
				if (action == AMOTION_EVENT_ACTION_CANCEL) m_touch.Reset();
				else if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN)
					m_touch.Down(AMotionEvent_getPointerId(event, index), AMotionEvent_getX(event, index), AMotionEvent_getY(event, index));
				else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP)
					m_touch.Release(AMotionEvent_getPointerId(event, index));
				else if (action == AMOTION_EVENT_ACTION_MOVE)
					for (size_t i = 0; i < AMotionEvent_getPointerCount(event); ++i)
						m_touch.Move(AMotionEvent_getPointerId(event, i), AMotionEvent_getX(event, i), AMotionEvent_getY(event, i));
				return;
			}
			m_mouse = { AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0) };
			if (action == AMOTION_EVENT_ACTION_CANCEL) Reset();
			else if (action == AMOTION_EVENT_ACTION_UP) m_mouse_buttons[0] = false;
			else if (action == AMOTION_EVENT_ACTION_DOWN) m_mouse_buttons[0] = true;
			const auto buttons = AMotionEvent_getButtonState(event);
			m_mouse_buttons[1] = (buttons & AMOTION_EVENT_BUTTON_SECONDARY) != 0;
			m_mouse_buttons[2] = (buttons & AMOTION_EVENT_BUTTON_TERTIARY) != 0;
		}
	}
}

#endif // DL_PLATFORM_ANDROID
