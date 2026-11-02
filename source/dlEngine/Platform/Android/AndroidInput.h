#pragma once

#include "Core/Input.h"
#include "Utils/TouchControls.h"
#include <array>

struct AInputEvent;

namespace Daedalus
{
	class AndroidInput final : public Input
	{
	public:
		AndroidInput();
		~AndroidInput() override;
		void HandleInput(AInputEvent* event);
		void Reset();
		static void DrawControls();

	protected:
		void UpdateImpl() override;
		bool IsKeyPressedImpl(int code) override;
		bool IsKeyHoldImpl(int code) override;
		bool IsKeyReleasedImpl(int code) override;
		bool IsMouseButtonPressedImpl(int button) override;
		std::pair<float, float> GetMousePosImpl() override { return m_mouse; }
		bool IsGpadKeyPressedImpl(int) override { return false; }
		std::pair<float, float> GetStickPosImpl(StickType) override { return { 0.f, 0.f }; }
		float GetBumperPosImpl(BamperType) override { return -1.f; }

	private:
		std::array<bool, DL_KEY_MENU + 1> m_pending{};
		std::array<bool, DL_KEY_MENU + 1> m_current{};
		std::array<bool, DL_KEY_MENU + 1> m_previous{};
		std::array<bool, 3> m_mouse_buttons{};
		std::pair<float, float> m_mouse{};
		TouchControls m_touch;
		std::array<bool, 2> m_toggle_pulses{};
	};
}
