#pragma once

#include <Daedalus.h>

class ExampleLayer final : public Daedalus::Layer
{
public:
	ExampleLayer() : Layer("Example") {}

	void OnAttach() override;
	void OnUpdate() override;

private:
	Daedalus::PerspectiveCamera m_camera;
	Daedalus::Model m_model;
};