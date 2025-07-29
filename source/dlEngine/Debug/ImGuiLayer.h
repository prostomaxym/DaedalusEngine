#pragma once

#include "Core/Layer.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#include <filesystem>

namespace Daedalus
{
	class DAEDALUS_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		virtual ~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		virtual void OnUpdate(DeltaTime dt) override;

		void BlockEvents(bool block) { m_block_events = block; }

		void SetDarkThemeColors();

		void Show(bool show) { m_show = show; }
		void Toggle() { m_show = !m_show; BlockEvents(!m_show); }
		bool IsShown() { return m_show; }

		static void LoadConfig();
		static void SaveConfig();
		void DoLoadConfig();

	protected:
		virtual void Update(DeltaTime dt) {};
		virtual void InitWindow() {};

	private:
		bool m_block_events = true;
		bool m_show = false;
	};
}