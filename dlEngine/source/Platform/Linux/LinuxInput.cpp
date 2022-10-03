#ifdef DL_PLATFORM_LINUX
#include "LinuxInput.h"

using namespace Daedalus;

Input* Input::s_instance = new LinuxInput();
#endif