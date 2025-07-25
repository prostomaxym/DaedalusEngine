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

		void RenderGeneralPage(DeltaTime dt);
		void RenderFPSSection(DeltaTime dt);
		void RenderCPUSection(DeltaTime dt);
		void RenderRAMSection(DeltaTime dt);

		float CalculateLowPercentile(float percentile) const;

		RingVector<float> m_frame_times{ GraphSize };
		RingVector<float> m_cpu_app_load{ GraphSize };
		RingVector<float> m_cpu_total_load{ GraphSize };
		std::vector<float> m_x_axe_values{ GraphSize };
	};
}