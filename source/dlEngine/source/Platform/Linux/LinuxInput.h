#pragma once 

#ifdef DL_PLATFORM_LINUX
#include "Platform/GLFW/GLFWInput.h"

namespace Daedalus {

	class LinuxInput : public GLFWInput {};
}
#endif