#pragma once

#ifdef DL_PLATFORM_ANDROID

#include "Core/Window.h"
#include "AndroidContext.h"
#include <cstdint>

namespace Daedalus
{
	class AndroidWindow final : public Window
	{
	public:
		explicit AndroidWindow(const WindowProps& props);
		~AndroidWindow() override;
		void SwapBuffers() override;
		void PollEvents() override;
		void* GetNativeWindow() const override;
		int GetWidth() const override { return m_width; }
		int GetHeight() const override { return m_height; }
		float GetAspectRatio() const override;
		void SetEventCallback(const EventCallbackFn& callback) override { m_callback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override { return m_vsync; }
		bool IsFullscreen() const override { return true; }
		void SetWindowMode(WindowProps::ScreenMode) override {}
		void FreeCursor(bool) override {}
		bool IsRenderable() const override;
		bool ConsumeFrameClockReset() override;
		bool IsContextLost() const { return m_context.IsLost(); }
		void OnCommand(int32_t command);

	private:
		void UpdateSize();
		AndroidContext m_context;
		EventCallbackFn m_callback;
		int m_width{ 0 };
		int m_height{ 0 };
		bool m_vsync{ true };
		bool m_reset_clock{ true };
		bool m_closed{ false };
	};
}

#endif // DL_PLATFORM_ANDROID
