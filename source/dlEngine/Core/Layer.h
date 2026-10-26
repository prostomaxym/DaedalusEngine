#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"
#include "Events/Event.h"

#include <string>

namespace Daedalus {

	class DAEDALUS_API Layer
	{
	public:
		explicit Layer(const std::string& name = "Layer", bool main_layer = false);
		virtual ~Layer() = default;
		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;
		Layer(Layer&&) = delete;
		Layer& operator=(Layer&&) = delete;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(DeltaTime dt) {};
		virtual void OnFixedUpdate(DeltaTime dt) {}
		virtual void OnRender(DeltaTime dt) {}
		virtual void OnImGuiRender(DeltaTime dt) {}
		virtual void OnEvent(Event& event) {};

		const std::string& GetName() const { return m_debug_name; }
		bool IsMain() { return m_main_layer; }

	protected:
		std::string m_debug_name;
		bool m_main_layer{ false };
	};
}