#pragma once

#include <GLFW/glfw3.h>

#include "Daedalus/IWindow.h"

namespace Daedalus {

class WindowsWindow : public IWindow
{
public:
	WindowsWindow(const WindowProps& props);
	virtual ~WindowsWindow();

	void OnUpdate() override;

	inline int GetWidth() const override { return m_data.width; }
	inline int GetHeight() const override { return m_data.height; }

	inline void SetEventCallback(const EventCallbackFn& callback) override { m_data.EventCallback = callback; }
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;

private:
	virtual void Init(const WindowProps& props);
	virtual void Shutdown();

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