#pragma once

#ifdef DL_PLATFORM_ANDROID

#include "Renderer/Primitives/GraphicsContext.h"
#include <EGL/egl.h>

struct ANativeWindow;

namespace Daedalus
{
	class AndroidContext final : public GraphicsContext
	{
	public:
		explicit AndroidContext(ANativeWindow* window);
		~AndroidContext() override;
		AndroidContext(const AndroidContext&) = delete;
		AndroidContext& operator=(const AndroidContext&) = delete;
		AndroidContext(AndroidContext&&) = delete;
		AndroidContext& operator=(AndroidContext&&) = delete;
		void Init() override;
		void SwapBuffers() override;
		bool Attach(ANativeWindow* window);
		void Detach();
		void SetVSync(bool enabled);
		bool IsLost() const { return m_lost; }
		bool HasSurface() const { return m_surface != EGL_NO_SURFACE; }

	private:
		ANativeWindow* m_initial_window; // NativeActivity owns the native window.
		EGLDisplay m_display{ EGL_NO_DISPLAY };
		EGLContext m_context{ EGL_NO_CONTEXT };
		EGLSurface m_surface{ EGL_NO_SURFACE };
		EGLSurface m_parking_surface{ EGL_NO_SURFACE };
		EGLConfig m_config{};
		bool m_lost{ false };
		bool m_vsync{ true };
	};
}

#endif // DL_PLATFORM_ANDROID
