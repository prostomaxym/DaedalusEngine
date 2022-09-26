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
	~ImGuiLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate() override;
	void OnEvent(Event& event) override;

private:
	bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& event);
	bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& event);
	bool OnMouseMovedEvent(MouseMovedEvent& event);
	bool OnMouseScrolledEvent(MouseScrolledEvent& event);

	bool OnKeyPressedEvent(KeyPressedEvent& event);
	bool OnKeyReleasedEvent(KeyReleasedEvent& event);
	bool OnKeyTypedEvent(KeyTypedEvent& event);

	bool OnWindowResizedEvent(WindowResizeEvent& event);

	double m_time;
};

}