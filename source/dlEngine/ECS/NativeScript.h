#pragma once

#include "Entity.h"
#include "Utils/DeltaTime.h"

namespace Daedalus 
{
	class Event;

	class DAEDALUS_API NativeScript
	{
	public:
		NativeScript(Entity entity) : m_entity(entity) {}
		virtual ~NativeScript() = default;

		template<typename T>
		T& GetComponent()
		{
			return m_entity.GetComponent<T>();
		}

	protected:
		virtual void OnUpdate(DeltaTime dt) {}
		virtual void OnEvent(const Event& event) {}
		virtual void OnRuntimeStop() {}

		Entity m_entity;
		friend class Scene;
	};
}
