#pragma once

#include "Core.h"

namespace Daedalus {

class DAEDALUS_API Application
{
public:
	Application();
	virtual ~Application();

	void Run();
};

//Define in client
Application* CreateApplication();
}


