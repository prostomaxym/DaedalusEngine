#pragma once

#include "Utils/DeltaTime.h"

namespace Daedalus
{
	class Event;

	class ImGuiWindow
	{		
	public:
		ImGuiWindow() = default;

		bool IsShown() { return m_show; }
		void Show(bool show) { m_show = show; }
		void Toggle() { m_show = !m_show; }

		virtual void Init() {};
		virtual void Update(DeltaTime dt) {};

		virtual void OnEvent(Event& evt) {};

	private:
		bool m_show = false;
	};
}