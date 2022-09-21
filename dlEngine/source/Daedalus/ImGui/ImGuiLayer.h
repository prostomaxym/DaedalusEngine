#pragma once

#include "Daedalus/Layer.h"

namespace Daedalus {

	class DAEDALUS_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& event) override;

	private:
		double m_time;
	};
}