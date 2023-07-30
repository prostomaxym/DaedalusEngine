#pragma once

#include "Entity.h"
#include "Daedalus/Utils/DeltaTime.h"

namespace Daedalus 
{
	class NativeScript
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

		Entity m_entity;
		friend class Scene;
	};
}
