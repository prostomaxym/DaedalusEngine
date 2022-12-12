#include "dlpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include <glad/glad.h>

namespace Daedalus {
	
	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         DL_CORE_CRITICAL(message); return;
			case GL_DEBUG_SEVERITY_MEDIUM:       DL_CORE_ERROR(message); return;
			case GL_DEBUG_SEVERITY_LOW:          DL_CORE_WARN(message); return;
			case GL_DEBUG_SEVERITY_NOTIFICATION: DL_CORE_TRACE(message); return;
		}
	}

	void OpenGLRendererAPI::Init()
	{
	#ifdef DL_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);
		
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
	}
}
