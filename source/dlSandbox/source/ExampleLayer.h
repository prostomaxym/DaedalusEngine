#pragma once

#include "Daedalus/Core/Layer.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"

class ExampleLayer final : public Daedalus::Layer
{
public:
	ExampleLayer() : Layer("Example") {}

	void OnAttach() override;
	void OnUpdate() override;

	void OnEvent(Daedalus::Event& event) override
	{
		DL_TRACE("{0}", event);
	}

private:
	Daedalus::PerspectiveCamera m_camera;
	Daedalus::Model m_model;
};