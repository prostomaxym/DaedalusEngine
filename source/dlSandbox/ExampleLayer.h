#pragma once

#include <Daedalus.h>

namespace Daedalus 
{
	class ExampleLayer final : public Layer
	{
	public:
		ExampleLayer() : Layer("Example", true) {}

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(DeltaTime dt) override;

		static ModelParserFlags s_enhance_model_flags;

	private:
		void PrepareNukeScene();
		void PrepareAnorLondoScene();
	};
}