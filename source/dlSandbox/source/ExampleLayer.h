#pragma once

#include <Daedalus.h>

class ExampleLayer final : public Daedalus::Layer
{
public:
	ExampleLayer() : Layer("Example") {}

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate() override;

private:
	Daedalus::Scene m_scene;
};