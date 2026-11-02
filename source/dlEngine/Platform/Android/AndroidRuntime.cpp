#include "AndroidRuntime.h"
#include "AndroidWindow.h"
#include "AndroidInput.h"
#include <android_native_app_glue.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <cstdlib>

namespace Daedalus
{
	AndroidRuntime* AndroidRuntime::s_instance = nullptr;

	AndroidRuntime::AndroidRuntime(android_app& app) : m_app(app)
	{
		if (s_instance) std::abort();
		s_instance = this;
		m_app.userData = this;
		m_app.onAppCmd = OnCommand;
		m_app.onInputEvent = OnInput;
	}

	AndroidRuntime::~AndroidRuntime()
	{
		m_app.onInputEvent = nullptr;
		m_app.onAppCmd = nullptr;
		m_app.userData = nullptr;
		s_instance = nullptr;
	}

	AndroidRuntime& AndroidRuntime::Get()
	{
		if (!s_instance) std::abort();
		return *s_instance;
	}

	bool AndroidRuntime::IsDestroying() const { return m_app.destroyRequested != 0; }
	bool AndroidRuntime::IsActive() const { return m_app.window && m_resumed && m_focused && !IsDestroying(); }

	void AndroidRuntime::PollEvents(bool wait)
	{
		android_poll_source* source = nullptr;
		int events = 0;
		int timeout = wait ? -1 : 0;
		while (!IsDestroying() && ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0)
		{
			if (source) source->process(&m_app, source);
			timeout = 0;
		}
	}

	void AndroidRuntime::OnCommand(android_app* app, int32_t command)
	{
		auto& runtime = *static_cast<AndroidRuntime*>(app->userData);
		if (command == APP_CMD_RESUME) runtime.m_resumed = true;
		if (command == APP_CMD_PAUSE) runtime.m_resumed = false;
		if (command == APP_CMD_GAINED_FOCUS) runtime.m_focused = true;
		if (command == APP_CMD_LOST_FOCUS) runtime.m_focused = false;
		if (!runtime.IsActive() && runtime.m_input) runtime.m_input->Reset();
		if (runtime.m_window) runtime.m_window->OnCommand(command);
	}

	int32_t AndroidRuntime::OnInput(android_app* app, AInputEvent* event)
	{
		auto& runtime = *static_cast<AndroidRuntime*>(app->userData);
		if (ImGui::GetCurrentContext()) ImGui_ImplAndroid_HandleInputEvent(event);
		if (runtime.m_input && runtime.IsActive()) runtime.m_input->HandleInput(event);
		return 1;
	}
}
