#pragma once

// ---Entry Point------------------------
#include "Core/EntryPoint.h"
// --------------------------------------

#include <Daedalus.h>

namespace Daedalus
{
	class GameApp final : public Application
	{
	public:
		GameApp();
		~GameApp() override = default;
	};
}
