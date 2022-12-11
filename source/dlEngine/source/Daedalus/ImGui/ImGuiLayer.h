#pragma once

#include "Daedalus/Core/Layer.h"
#include "Daedalus/Events/ApplicationEvent.h"
#include "Daedalus/Events/KeyEvent.h"
#include "Daedalus/Events/MouseEvent.h"


namespace Daedalus {

class DAEDALUS_API ImGuiLayer : public Layer
{
public:
	ImGuiLayer();
	virtual ~ImGuiLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(Event& e) override;

	void Begin();
	void End();

	void BlockEvents(bool block) { m_block_events = block; }

	void SetDarkThemeColors();
private:
	bool m_block_events = true;
};

}