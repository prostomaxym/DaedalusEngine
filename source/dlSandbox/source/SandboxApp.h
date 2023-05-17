#pragma once

// ---Entry Point------------------------
#include "Daedalus/Core/EntryPoint.h"
// --------------------------------------

#include "Daedalus/Utils/HighPerformanceRequest.h"

#include <Daedalus.h>


class Sandbox : public Daedalus::Application
{
public:
	static Application* GetInstance()
	{
		if (!s_instance)
		{
			s_instance = new Sandbox();
		}

		return s_instance;
	}

	~Sandbox() = default;

private:
	Sandbox();
};