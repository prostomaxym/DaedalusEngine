#pragma once

#include <Daedalus.h>

namespace Daedalus 
{
	class ExampleLayer final : public Layer
	{
	public:
		ExampleLayer() : Layer("Example") {}

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;

	private:
		Scene m_scene;
	};
}