#pragma once

#include "Macros.h"
#include <cstdint>

struct android_app;
struct AInputEvent;

namespace Daedalus
{
	class AndroidWindow;
	class AndroidInput;

	class AndroidRuntime
	{
	public:
		explicit AndroidRuntime(android_app& app);
		~AndroidRuntime();
		AndroidRuntime(const AndroidRuntime&) = delete;
		AndroidRuntime& operator=(const AndroidRuntime&) = delete;
		AndroidRuntime(AndroidRuntime&&) = delete;
		AndroidRuntime& operator=(AndroidRuntime&&) = delete;

		static AndroidRuntime& Get();
		void PollEvents(bool wait);
		bool IsActive() const;
		bool IsDestroying() const;
		android_app& GetApp() const { return m_app; }
		void SetWindow(AndroidWindow* window) { m_window = window; }
		void SetInput(AndroidInput* input) { m_input = input; }

	private:
		static void OnCommand(android_app* app, int32_t command);
		static int32_t OnInput(android_app* app, AInputEvent* event);
		static AndroidRuntime* s_instance; // Owned by the native entry's stack.
		android_app& m_app;
		AndroidWindow* m_window{ nullptr }; // Engine owns the window.
		AndroidInput* m_input{ nullptr }; // Engine owns the input backend.
		bool m_resumed{ false };
		bool m_focused{ false };
	};
}
