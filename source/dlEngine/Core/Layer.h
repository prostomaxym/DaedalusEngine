#pragma once

#include "Macros.h"
#include "ECS/Scene.h"
#include "Events/Event.h"

#include <string>

namespace Daedalus {

	class DAEDALUS_API Layer
	{
	public:
		explicit Layer(const std::string& name = "Layer", bool main_layer = false);
		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(DeltaTime dt) {};
		virtual void OnEvent(Event& event) {};

		const std::string& GetName() const { return m_debug_name; }
		Scene* GetScene() { return &m_scene; }
		bool IsMain() { return m_main_layer; }

	protected:
		std::string m_debug_name;
		Scene m_scene;
		bool m_main_layer{ false };
	};
}