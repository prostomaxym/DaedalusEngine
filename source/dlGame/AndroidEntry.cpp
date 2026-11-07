#include "Platform/Android/AndroidBootstrap.h"
#include "Core/Application.h"

extern "C" void android_main(android_app* app)
{
	Daedalus::RunAndroidApplication(app, &Daedalus::CreateApplication);
}
