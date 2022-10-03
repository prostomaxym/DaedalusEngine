#ifdef DL_PLATFORM_WINDOWS
#include "WindowsInput.h"

using namespace Daedalus;

Input* Input::s_instance = new WindowsInput();
#endif