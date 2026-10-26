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

	private:
		ScriptRegistry m_scripts;
	};
}