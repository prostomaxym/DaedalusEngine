#pragma once

#include "Core/Layer.h"
#include "Events/Event.h"
#include "ImGuiWindow.h"

#include <filesystem>
#include <vector>

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

		static void LoadConfig();
		static void SaveConfig();
		void DoLoadConfig();

		void AddWindow(std::unique_ptr<ImGuiWindow>&& window);

	private:
		std::vector<std::unique_ptr<ImGuiWindow>> m_windows;
		bool m_block_events = true;	
	};
}