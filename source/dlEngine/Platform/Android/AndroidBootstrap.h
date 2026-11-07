#pragma once

#ifdef DL_PLATFORM_ANDROID

#include "Macros.h"

struct android_app;

namespace Daedalus
{
	class Application;
	DAEDALUS_API void RunAndroidApplication(android_app* app, Application* (*create_application)());
}

#endif // DL_PLATFORM_ANDROID
