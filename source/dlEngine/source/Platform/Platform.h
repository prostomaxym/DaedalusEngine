#pragma once

#include <Daedalus/Core/Window.h>
#include <Daedalus/Core/Input.h>

#include <memory>

namespace Daedalus {

	class Platform
	{
	public:
		static std::unique_ptr<Window> createWindow();
		static void InitInputSystem();
	};

}
