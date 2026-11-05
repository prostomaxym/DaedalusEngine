#pragma once

#include "Macros.h"
#include "Utils/KeyCodes.h"
#include "Config/KeybindConfig.h"

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

		static void Update();
		[[nodiscard]] static bool IsAvailable() { return s_instance != nullptr; }
		[[nodiscard]] static bool IsActionDown(const std::string& name);
		[[nodiscard]] static bool IsActionPressed(const std::string& name);
		[[nodiscard]] static bool IsActionHold(const std::string& name);
		[[nodiscard]] static bool IsActionReleased(const std::string& name);
		[[nodiscard]] static float GetAxis(const std::string& name);
		[[nodiscard]] static float GetMovementAction(const std::string& name_positive, const std::string& name_negative);

		//Input configuration
		static void EnableKeyboardInput(bool enable);
		static void EnableGamepadInput(bool enable);
		static void SetSticksDeadZone(float perc);

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

		inline static bool IsGpadKeyPressed(int keycode) { return IsGamepadInputEnabled() && keycode >= 0 && keycode <= DL_GAMEPAD_BUTTON_LAST && GetInstance()->IsGpadKeyPressedImpl(keycode); }
		static std::pair<float, float> GetStickPos(StickType type);
		inline static float GetBumperPos(BamperType type) { return IsGamepadInputEnabled() ? GetInstance()->GetBumperPosImpl(type) : 0.f; }

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
		struct ActionState
		{
			InputActionBinding binding;
			bool keyboard{ false };
			bool gamepad{ false };
			bool previous{ false };
			bool down{ false };
		};
		void RefreshActionGates();
		float ReadGamepadAxis(GamepadAxis axis);
		static Input* s_instance; // Non-owning; Engine owns the platform input system.
		std::unordered_map<std::string, ActionState> m_action_states;
		uint64_t m_binding_revision{ 0 };
		bool m_keyboard_connected{ true };
		bool m_gamepad_connected{ true };
		float m_sticks_deadzone{ 0.15f };
	};
}
