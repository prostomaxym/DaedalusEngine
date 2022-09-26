#pragma once 

#ifdef DL_PLATFORM_WINDOWS
#include "Platform/GLFW/GLFWInput.h"

namespace Daedalus {

	class WindowsInput : public GLFWInput {};
}
#endif