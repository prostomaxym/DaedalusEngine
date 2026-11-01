#pragma once

#include "Core/Layer.h"
#include "ImGuiWindow.h"

#include <memory>
#include <string>
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

		void BeginFrame();
		void EndFrame();
		void OnImGuiRender(DeltaTime dt) override;

		void BlockEvents(bool block) { m_block_events = block; }

		void SetDarkThemeColors();

		static void LoadConfig();
		static void SaveConfig();
		void DoLoadConfig();

		void AddWindow(std::unique_ptr<ImGuiWindow>&& window);

	private:
		// ImGui borrows these bytes until its font atlas/context is destroyed.
		std::string m_font_data;
		std::vector<std::unique_ptr<ImGuiWindow>> m_windows;
		bool m_block_events = true;	
	};
}