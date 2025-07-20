#pragma once

#include "Entity.h"
#include "NativeScript.h"

namespace Daedalus
{
	struct NativeScriptComponent
	{
		template <typename T, typename... Args>
		T* AddScript(Entity ent, Args&&... args)
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			DL_ASSERT(!ScriptExists<T>(), Log::Categories::ECS, "NativeScriptComponent already has this script!");

			native_scripts[typeid(T)] = std::make_shared<T>(ent, std::forward<Args>(args)...);
			return dynamic_cast<T*>(native_scripts[typeid(T)].get());
		}

		template <typename T>
		void RemoveScript()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			DL_ASSERT(ScriptExists<T>(), Log::Categories::ECS, "NativeScriptComponent does not have this script!");

			native_scripts.erase(typeid(T));
		}

		template <typename T>
		T* GetScript()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");

			const auto it = native_scripts.find(typeid(T));
			DL_ASSERT(it != native_scripts.end(), Log::Categories::ECS, "NativeScriptComponent does not have this script!");
			return it != native_scripts.end() ? dynamic_cast<T*>(it->second.get()) : nullptr;
		}

		template <typename T>
		bool ScriptExists()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			return native_scripts.find(typeid(T)) != native_scripts.end();
		}

		// TODO: It is not compiling with unique_ptr, something inside entt tries to copy it.
		// Review and replace it with unique_ptr or convert to static polymorhism
		std::unordered_map<std::type_index, std::shared_ptr<NativeScript>> native_scripts;
	};
}
