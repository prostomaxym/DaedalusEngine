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
		void OnUpdate(DeltaTime dt) override;

	private:
		Scene m_scene;
	};
}