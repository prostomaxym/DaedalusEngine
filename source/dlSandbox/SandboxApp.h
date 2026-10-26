#pragma once

// ---Entry Point------------------------
#include "Core/EntryPoint.h"
// --------------------------------------

#include "Utils/HighPerformanceRequest.h"

#include <Daedalus.h>

namespace Daedalus
{
	class Sandbox : public Application
	{
	public:
		Sandbox();
		~Sandbox() override = default;
	};
}
