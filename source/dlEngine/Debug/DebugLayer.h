#pragma once

#include "ImGuiLayer.h"

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
		static const int FrametimeGraphSize = 500;

		void RenderFPSSection(DeltaTime dt);
		float CalculateLowPercentile(float percentile) const;

		std::vector<float> m_frame_times{};
		std::vector<float> m_x_axe_values{};
	};
}