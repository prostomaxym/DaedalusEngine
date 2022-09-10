#pragma once


#include <GLFW/glfw3.h>

#include "Daedalus/IWindow.h"

namespace Daedalus {

	class GLFWWindow : public IWindow
	{
	public:
		GLFWWindow(const WindowProps& props);
		virtual ~GLFWWindow();

		void OnUpdate() override;

		inline int GetWidth() const override { return m_data.width; }
		inline int GetHeight() const override { return m_data.height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		virtual void SetupCallbacks();

	private:
		GLFWwindow* m_window;

		struct WindowData
		{
			std::string title;
			unsigned int width, height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_data;
	};

}