#pragma once

#include "Daedalus/Core/Window.h"
#include "Daedalus/Renderer/Primitives/GraphicsContext.h"

#include <memory>
#include <string>

#include <GLFW/glfw3.h>

namespace Daedalus {

	class GLFWWindow : public Window
	{
	public:
		GLFWWindow(const WindowProps& props);
		virtual ~GLFWWindow();

		void OnUpdate() override;

		inline void* GetNativeWindow() const override { return m_window; }
		inline int GetWidth() const override { return m_data.width; }
		inline int GetHeight() const override { return m_data.height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		bool IsFullscreen() const override;
		virtual void SetWindowMode(WindowProps::ScreenMode screenmode) override;

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		virtual void SetupCallbacks();

	private:

		std::unique_ptr<GraphicsContext> m_context;
		GLFWwindow* m_window;

		struct WindowData
		{
			std::string title;
			unsigned int width = 0u, height = 0u;
			bool VSync{ false };
			bool focused{ true };

			EventCallbackFn EventCallback;
		};

		WindowData m_data;
		WindowProps::ScreenMode m_screenmode {WindowProps::ScreenMode::fullscreen};
	};
}