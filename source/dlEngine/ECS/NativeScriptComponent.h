#pragma once

#include "Entity.h"
#include "NativeScript.h"
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>

namespace Daedalus
{
	struct DAEDALUS_API NativeScriptComponent
	{
		// Explicitly move-only: an exported (dllexport) class instantiates all implicit special members,
		// which would otherwise try to copy the unique_ptr map.
		NativeScriptComponent() = default;
		~NativeScriptComponent() = default;
		NativeScriptComponent(const NativeScriptComponent&) = delete;
		NativeScriptComponent& operator=(const NativeScriptComponent&) = delete;
		NativeScriptComponent(NativeScriptComponent&&) noexcept = default;
		NativeScriptComponent& operator=(NativeScriptComponent&&) noexcept = default;

		template <typename T, typename... Args>
		T* AddScript(Entity ent, Args&&... args)
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			DL_ASSERT(!ScriptExists<T>(), Log::Categories::ECS, "NativeScriptComponent already has this script!");

			auto script = std::make_unique<T>(ent, std::forward<Args>(args)...);
			T* raw_script = script.get();
			native_scripts[typeid(T)] = std::move(script);
			return raw_script;
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

		std::unordered_map<std::type_index, std::unique_ptr<NativeScript>> native_scripts;
	};
}
