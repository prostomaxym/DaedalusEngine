#include "AndroidWindow.h"
#include "AndroidRuntime.h"
#include "Events/ApplicationEvent.h"
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <utility>

namespace Daedalus
{
	AndroidWindow::AndroidWindow(const WindowProps& props)
		: m_context(AndroidRuntime::Get().GetApp().window), m_vsync(props.vsync)
	{
		m_context.Init();
		m_context.SetVSync(m_vsync);
		AndroidRuntime::Get().SetWindow(this);
		UpdateSize();
	}

	AndroidWindow::~AndroidWindow() { AndroidRuntime::Get().SetWindow(nullptr); }
	void* AndroidWindow::GetNativeWindow() const { return AndroidRuntime::Get().GetApp().window; }
	float AndroidWindow::GetAspectRatio() const { return m_height > 0 ? static_cast<float>(m_width) / m_height : 1.f; }
	bool AndroidWindow::IsRenderable() const { return AndroidRuntime::Get().IsActive() && m_context.HasSurface() && !m_context.IsLost(); }
	bool AndroidWindow::ConsumeFrameClockReset() { return std::exchange(m_reset_clock, false); }
	void AndroidWindow::SetVSync(bool enabled) { m_vsync = enabled; m_context.SetVSync(enabled); }
	void AndroidWindow::SwapBuffers() { if (IsRenderable()) m_context.SwapBuffers(); }

	void AndroidWindow::PollEvents()
	{
		auto& runtime = AndroidRuntime::Get();
		runtime.PollEvents(!IsRenderable() && !m_context.IsLost());
		if ((runtime.IsDestroying() || m_context.IsLost()) && !m_closed && m_callback)
		{
			m_closed = true;
			WindowCloseEvent event;
			m_callback(event);
		}
	}

	void AndroidWindow::UpdateSize()
	{
		auto* window = static_cast<ANativeWindow*>(GetNativeWindow());
		if (!window) return;
		m_width = ANativeWindow_getWidth(window);
		m_height = ANativeWindow_getHeight(window);
		if (m_callback && m_width > 0 && m_height > 0)
		{
			WindowResizeEvent event(m_width, m_height);
			m_callback(event);
		}
	}

	void AndroidWindow::OnCommand(int32_t command)
	{
		switch (command)
		{
		case APP_CMD_INIT_WINDOW:
			if (m_context.Attach(static_cast<ANativeWindow*>(GetNativeWindow())))
			{
				if (ImGui::GetCurrentContext()) ImGui_ImplAndroid_Init(static_cast<ANativeWindow*>(GetNativeWindow()));
				UpdateSize();
			}
			m_reset_clock = true;
			break;
		case APP_CMD_TERM_WINDOW:
			m_context.Detach();
			m_reset_clock = true;
			break;
		case APP_CMD_WINDOW_RESIZED:
		case APP_CMD_CONTENT_RECT_CHANGED:
			UpdateSize();
			break;
		case APP_CMD_PAUSE:
		case APP_CMD_RESUME:
		case APP_CMD_LOST_FOCUS:
		case APP_CMD_GAINED_FOCUS:
			m_reset_clock = true;
			break;
		}
	}
}
