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

		inline static bool IsGpadKeyPressed(int keycode) { return s_instance->IsGpadKeyPresseImpl(keycode); }
		inline std::pair<float, float> GetStickPos(StickType type) { return s_instance->GetStickPosImpl(type); }
		inline static float GetBumperPos(BamperType type) { return s_instance->GetBumperPosImpl(type); }

	protected:
		static void CreateInstance(Input* inst) { s_instance = inst; }

		// Keyboard + mouse input
		virtual bool IsKeyPressedImpl(int keycode) = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual std::pair<float, float> GetMousePosImpl() = 0;

		//Gamepad input
		virtual bool IsGpadKeyPresseImpl(int keycode) = 0;
		virtual std::pair<float, float> GetStickPosImpl(StickType type) = 0;
		virtual float GetBumperPosImpl(BamperType type) = 0;

	private:
		static Input* s_instance;
		bool m_has_mouse_keyboard = false;
		bool m_has_gamepad = false;
	};

}