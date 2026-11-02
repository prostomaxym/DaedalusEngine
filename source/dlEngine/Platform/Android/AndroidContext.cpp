#include "AndroidContext.h"
#include "Debug/Log.h"
#include <android/native_window.h>
#include <cstdlib>

namespace Daedalus
{
	AndroidContext::AndroidContext(ANativeWindow* window) : m_initial_window(window) {}

	void AndroidContext::Init()
	{
		m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		EGLint count = 0;
		const EGLint attributes[]{ EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_NONE };
		const EGLint context_attributes[]{ EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE };
		const EGLint parking_attributes[]{ EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
		if (m_display != EGL_NO_DISPLAY && eglInitialize(m_display, nullptr, nullptr) &&
			eglBindAPI(EGL_OPENGL_ES_API) && eglChooseConfig(m_display, attributes, &m_config, 1, &count) && count)
		{
			m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, context_attributes);
			m_parking_surface = eglCreatePbufferSurface(m_display, m_config, parking_attributes);
		}
		if (m_context == EGL_NO_CONTEXT || m_parking_surface == EGL_NO_SURFACE || !Attach(m_initial_window))
		{
			Log::Write(Log::Levels::Critical, Log::Categories::Platform, "Cannot initialize Android GLES 3.2 context: EGL error {:#x}", eglGetError());
			std::abort();
		}
		m_initial_window = nullptr;
	}

	AndroidContext::~AndroidContext()
	{
		if (m_display == EGL_NO_DISPLAY) return;
		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
		if (m_parking_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_parking_surface);
		if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
		eglTerminate(m_display);
	}

	bool AndroidContext::Attach(ANativeWindow* window)
	{
		if (!window || m_lost) return false;
		Detach();
		EGLint format = 0;
		eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &format);
		ANativeWindow_setBuffersGeometry(window, 0, 0, format);
		m_surface = eglCreateWindowSurface(m_display, m_config, window, nullptr);
		if (m_surface == EGL_NO_SURFACE || !eglMakeCurrent(m_display, m_surface, m_surface, m_context))
		{
			const auto error = eglGetError();
			m_lost = error == EGL_CONTEXT_LOST;
			Log::Write(Log::Levels::Error, Log::Categories::Platform, "Cannot attach Android surface: EGL error {:#x}", error);
			Detach();
			return false;
		}
		eglSwapInterval(m_display, m_vsync ? 1 : 0);
		return true;
	}

	void AndroidContext::Detach()
	{
		// Keep the context current for orderly GPU-resource destruction even without a window.
		if (m_context != EGL_NO_CONTEXT && !m_lost && m_parking_surface != EGL_NO_SURFACE)
		{
			if (!eglMakeCurrent(m_display, m_parking_surface, m_parking_surface, m_context))
				m_lost = eglGetError() == EGL_CONTEXT_LOST;
		}
		if (m_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(m_display, m_surface);
			m_surface = EGL_NO_SURFACE;
		}
	}

	void AndroidContext::SwapBuffers()
	{
		if (HasSurface() && !eglSwapBuffers(m_display, m_surface))
		{
			const auto error = eglGetError();
			m_lost = error == EGL_CONTEXT_LOST;
			Log::Write(Log::Levels::Error, Log::Categories::Platform, "Android swap failed: EGL error {:#x}", error);
			Detach();
		}
	}

	void AndroidContext::SetVSync(bool enabled)
	{
		m_vsync = enabled;
		if (HasSurface()) eglSwapInterval(m_display, enabled ? 1 : 0);
	}
}
