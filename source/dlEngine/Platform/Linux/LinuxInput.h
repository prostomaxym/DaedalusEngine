#pragma once 

#ifdef DL_PLATFORM_LINUX
#include "Platform/GLFW/GLFWInput.h"

namespace Daedalus {

	class LinuxInput final : public GLFWInput {};
}
#endif