#pragma once

#include "Entity.h"
#include "Daedalus/Utils/DeltaTime.h"

namespace Daedalus 
{
	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() {}

		template<typename T>
		T& GetComponent()
		{
			return m_entity.GetComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(DeltaTime dt) {}

	private:
		Entity m_entity{};
		friend class Scene;
	};
}
