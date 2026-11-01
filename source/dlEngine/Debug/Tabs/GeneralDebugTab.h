#pragma once

#include "Utils/DeltaTime.h"
#include "Utils/RingVector.h"

#include <string>
#include <vector>

namespace Daedalus
{
	class GeneralDebugTab
	{
	public:
		void Init();
		void Render(DeltaTime dt);

	private:
		static constexpr int GraphSize = 500;
		static constexpr int LastSecondIdx = GraphSize * 4 / 5;
		static inline float NumberOfSecondsTracked = 10.f;

		void RenderFPSSection(DeltaTime dt);
		void RenderCPUSection(DeltaTime dt);
		void RenderRAMSection(DeltaTime dt);
		void RenderWorldSection();
		void RenderGPUSection(DeltaTime dt);
		float CalculateLowPercentile(float percentile) const;

		RingVector<float> m_frame_times{ GraphSize };
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
	};
}
