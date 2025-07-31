#pragma once

#include "ImGuiWindow.h"

#include "Events/KeyEvent.h"

#include "Utils/RingVector.h"

namespace Daedalus
{
	class DAEDALUS_API DebugOverlay : public ImGuiWindow
	{
	public:
		DebugOverlay() = default;
		virtual ~DebugOverlay() = default;

		bool OnKeyReleased(KeyReleasedEvent& event);
		virtual void OnEvent(Event& evt) override;

	private:
		void Update(DeltaTime dt) override;
		void Init() override;

	private:
		static const int GraphSize = 500;
		static const int LastSecondIdx = GraphSize * 4 / 5;
		static inline float NumberOfSecondsTracked = 10.f;

		void RenderGeneralPage(DeltaTime dt);
		void RenderFPSSection(DeltaTime dt);
		void RenderCPUSection(DeltaTime dt);
		void RenderRAMSection(DeltaTime dt);
		void RenderWorldSection();
		void RenderGPUSection(DeltaTime dt);

		void RenderRenderingPage();
		void RenderTexture(std::string_view name, uint32_t texture_id, ImVec2 size, bool use_aspect = true);

		void RenderLogsPage();

		float CalculateLowPercentile(float percentile) const;
	

	private:
		RingVector<float, GraphSize> m_frame_times;

		RingVector<float, GraphSize> m_cpu_app_load;
		RingVector<float, GraphSize> m_cpu_total_load;
		RingVector<float, GraphSize> m_ram_app_load;
		RingVector<float, GraphSize> m_ram_total_load;

		RingVector<float, GraphSize> m_gpu_utillization;
		RingVector<float, GraphSize> m_gpu_memory_utillization;
		RingVector<float, GraphSize> m_gpu_memory_used;

		std::vector<float> m_x_axe_values{ GraphSize };
		float m_total_ram_GB{ 0.f };
		float m_total_gpu_ram_GB{ 0.f };
		std::string m_processor_name;
		std::string m_gpu_name;

	private:
		class LogFilterPanel
		{
		public:
			LogFilterPanel();

			void Draw();

			bool IsLevelEnabled(Log::Levels level) const;
			bool IsCategoryEnabled(Log::Categories category_bit) const;

		private:
			std::array<bool, 5> level_flags_;

			void DrawCategoryCheckbox(Log::Categories cat);
			void DrawLevelCheckbox(Log::Levels level);
		};

		LogFilterPanel m_log_selection_panel;
	};
}