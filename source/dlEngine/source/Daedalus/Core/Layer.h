#pragma once

#include "Core.h"
#include "Daedalus/Events/Event.h"

#include <string>

namespace Daedalus {

	class DAEDALUS_API Layer
	{
	public:
		explicit Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(DeltaTime dt) {};
		virtual void OnEvent(Event& event) {};

		inline const std::string& GetName() const { return m_debug_name; }
	protected:
		std::string m_debug_name;
	};

}