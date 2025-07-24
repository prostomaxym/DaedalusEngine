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
		void ConstructWindow() override;

	private:
		static const int FrametimeGraphSize = 200;

		void RenderFPSSection(DeltaTime dt);
		float CalculateLowPercentile(float percentile) const;

		std::array<float, FrametimeGraphSize> m_frame_times;
	};
}