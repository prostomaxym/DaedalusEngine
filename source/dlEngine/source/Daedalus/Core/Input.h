#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Utils/KeyCodes.h"

#include <tuple>

namespace Daedalus {

	class Platform;

	class DAEDALUS_API Input
	{
		friend Platform;
	public:
		//Input configuration
		inline static void EnableKeyboardInput(bool enable) { m_keyboard_connected = enable; } //TODO: not implemented
		inline static void EnableGamepadInput(bool enable) { m_gamepad_connected = enable; }
		inline static void SetSticksDeadZone(float perc) { m_sticks_deadzone = perc; }

		inline static bool IsKeyboardInputEnabled() { return m_keyboard_connected; }
		inline static bool IsGamepadInputEnabled() { return m_gamepad_connected; }

		// Keyboard + mouse input
		inline static bool IsKeyPressed(int keycode) { return s_instance->IsKeyPressedImpl(keycode); }

		inline static bool IsMouseButtonPressed(int button) { return s_instance->IsMouseButtonPressedImpl(button); }
		inline static std::pair<float, float> GetMousePos() { return s_instance->GetMousePosImpl(); }


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

		inline static bool IsGpadKeyPressed(int keycode) { return s_instance->IsGpadKeyPressedImpl(keycode); }
		static std::pair<float, float> GetStickPos(StickType type);
		inline static float GetBumperPos(BamperType type) { return s_instance->GetBumperPosImpl(type); }

	protected:
		static void CreateInstance(Input* inst) { s_instance = inst; }

		// Keyboard + mouse input
		virtual bool IsKeyPressedImpl(int keycode) = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual std::pair<float, float> GetMousePosImpl() = 0;

		//Gamepad input
		virtual bool IsGpadKeyPressedImpl(int keycode) = 0;
		virtual std::pair<float, float> GetStickPosImpl(StickType type) = 0;
		virtual float GetBumperPosImpl(BamperType type) = 0;

	private:
		static Input* s_instance;
		static bool m_keyboard_connected;
		static bool m_gamepad_connected;
		static float m_sticks_deadzone;
	};
}
