#pragma once

#include <Core/Layer.h>
#include <ECS/Serialization/ScriptRegistry.h>

namespace Daedalus
{
	class GameLayer final : public Layer
	{
	public:
		GameLayer() : Layer("Game", true) {}

		void OnAttach() override;
		void OnDetach() override;

	private:
		ScriptRegistry m_scripts;
	};
}