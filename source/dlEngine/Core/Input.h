#pragma once

#include "Macros.h"
#include "Utils/KeyCodes.h"

#include <tuple>

namespace Daedalus {

	class Platform;
	class Engine;

	class DAEDALUS_API Input
	{
		friend Platform;
		friend Engine;
	public:
		Input();
		virtual ~Input();
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
		Input(Input&&) = delete;
		Input& operator=(Input&&) = delete;

		static void Update() { return GetInstance()->UpdateImpl(); }

		//Input configuration
		inline static void EnableKeyboardInput(bool enable) { GetInstance()->m_keyboard_connected = enable; }
		inline static void EnableGamepadInput(bool enable) { GetInstance()->m_gamepad_connected = enable; }
		inline static void SetSticksDeadZone(float perc) { GetInstance()->m_sticks_deadzone = perc; }

		inline static bool IsKeyboardInputEnabled() { return GetInstance()->m_keyboard_connected; }
		inline static bool IsGamepadInputEnabled() { return GetInstance()->m_gamepad_connected; }

		// Keyboard + mouse input
		inline static bool IsKeyPressed(int keycode) { return IsKeyboardInputEnabled() && GetInstance()->IsKeyPressedImpl(keycode); }
		inline static bool IsKeyHold(int keycode) { return IsKeyboardInputEnabled() && GetInstance()->IsKeyHoldImpl(keycode); }
		inline static bool IsKeyReleased(int keycode) { return IsKeyboardInputEnabled() && GetInstance()->IsKeyReleasedImpl(keycode); }

		inline static bool IsMouseButtonPressed(int button) { return GetInstance()->IsMouseButtonPressedImpl(button); }
		inline static std::pair<float, float> GetMousePos() { return GetInstance()->GetMousePosImpl(); }


		//Gamepad input
		enum class StickType
		{
			Left,
			Right
		};

		enum class BamperType
		{
			Left,
			Right
		};

		inline static bool IsGpadKeyPressed(int keycode) { return GetInstance()->IsGpadKeyPressedImpl(keycode); }
		static std::pair<float, float> GetStickPos(StickType type);
		inline static float GetBumperPos(BamperType type) { return GetInstance()->GetBumperPosImpl(type); }

	protected:
		static Input* GetInstance();
		virtual void UpdateImpl() = 0;

		// Keyboard + mouse input
		virtual bool IsKeyPressedImpl(int keycode) = 0;
		virtual bool IsKeyHoldImpl(int keycode) = 0;
		virtual bool IsKeyReleasedImpl(int keycode) = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual std::pair<float, float> GetMousePosImpl() = 0;

		//Gamepad input
		virtual bool IsGpadKeyPressedImpl(int keycode) = 0;
		virtual std::pair<float, float> GetStickPosImpl(StickType type) = 0;
		virtual float GetBumperPosImpl(BamperType type) = 0;

	private:
		static Input* s_instance;
		bool m_keyboard_connected{ true };
		bool m_gamepad_connected{ false };
		float m_sticks_deadzone{ 0.15f };
	};
}
