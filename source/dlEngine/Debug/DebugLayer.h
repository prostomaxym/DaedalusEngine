#pragma once

#include "ImGuiLayer.h"

#include "Utils/RingVector.h"

namespace Daedalus
{
	class DAEDALUS_API DebugLayer : public ImGuiLayer
	{
	public:
		DebugLayer() = default;
		virtual ~DebugLayer() = default;

	protected:
		void Update(DeltaTime dt) override;
		void InitWindow() override;

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
		void RenderTexture(std::string_view name, uint32_t texture_id);

		float CalculateLowPercentile(float percentile) const;

		RingVector<float> m_frame_times{ GraphSize };

		RingVector<float> m_cpu_app_load{ GraphSize };
		RingVector<float> m_cpu_total_load{ GraphSize };
		RingVector<float> m_ram_app_load{ GraphSize };
		RingVector<float> m_ram_total_load{ GraphSize };

		RingVector<float> m_gpu_utillization{ GraphSize };
		RingVector<float> m_gpu_memory_utillization{ GraphSize };
		RingVector<float> m_gpu_memory_used{ GraphSize };

		std::vector<float> m_x_axe_values{ GraphSize };
		float m_total_ram_GB{ 0.f };
		float m_total_gpu_ram_GB{ 0.f };
		std::string m_processor_name;
		std::string m_gpu_name;
	};
}