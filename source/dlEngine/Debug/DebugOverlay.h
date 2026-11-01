#pragma once

#include "ImGuiWindow.h"
#include <memory>

namespace Daedalus
{
	class KeyReleasedEvent;
	class GeneralDebugTab;
	class RenderingDebugTab;
	class ResourcesDebugTab;
	class ProfilerDebugTab;
	class LoggingDebugTab;

	class DAEDALUS_API DebugOverlay : public ImGuiWindow
	{
	public:
		DebugOverlay();
		virtual ~DebugOverlay();

		void Init() override;
		void Update(DeltaTime dt) override;

		void OnEvent(Event& evt) override;
		bool OnKeyReleased(KeyReleasedEvent& event);

	private:
		std::unique_ptr<GeneralDebugTab> m_general_tab;
		std::unique_ptr<RenderingDebugTab> m_rendering_tab;
		std::unique_ptr<ResourcesDebugTab> m_resources_tab;
		std::unique_ptr<ProfilerDebugTab> m_profiler_tab;
		std::unique_ptr<LoggingDebugTab> m_logging_tab;
		bool m_logs_tab_active = false;
	};
}