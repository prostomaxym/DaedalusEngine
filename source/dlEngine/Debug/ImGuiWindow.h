#pragma once

#include "Events/Event.h"
#include "Utils/DeltaTime.h"

#include <ImGui.h>

namespace Daedalus
{
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