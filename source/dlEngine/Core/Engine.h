#pragma once

#include "Macros.h"
#include <memory>

namespace Daedalus
{
	class Window;
	class EventEngine;
	class SceneManager;

	class DAEDALUS_API Engine
	{
	public:
		Engine();
		~Engine();
		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(Engine&&) = delete;

		Window& GetWindow();
		EventEngine& GetEvents();
		SceneManager& GetScenes();
		void Stop();

	private:
		struct State;
		std::unique_ptr<State> m_state;
	};
}
