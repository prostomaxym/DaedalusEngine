#pragma once

#include "dlpch.h"

#include "Core.h"
#include "Daedalus/Events/Event.h"

namespace Daedalus {

	struct WindowProps
	{
		std::string title;
		int width;
		int height;
		bool full_screen;
		bool vsync;

		explicit WindowProps(const std::string& title_ = "Daedalus Engine", int width_ = 1920, int height_ = 1080, bool full_screen_ = true, bool vsync_ = false) :
			title(title_), width(width_),
			height(height_),
			full_screen(full_screen_),
			vsync(vsync_)
		{
		}
	};

	class DAEDALUS_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual void* GetNativeWindow() const = 0;
		virtual int GetWidth() const = 0;
		virtual int GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual bool IsFullscreen() const = 0;
		virtual void SetFullscreen(bool enable) = 0;
	};

}
