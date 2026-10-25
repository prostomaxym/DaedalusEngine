#pragma once

#include "SceneRecord.h"
#include "ECS/NativeScriptComponent.h"

#include <functional>
#include <map>
#include <typeindex>

namespace Daedalus
{
	class DAEDALUS_API ScriptRegistry
	{
	public:
		using Validator = std::function<bool(const ScriptRecord&, const EntityRecord&, std::string&)>;

		template<typename T>
		bool Register(const std::string& name, Validator validate,
			std::function<T*(Entity, const ScriptRecord&)> create,
			std::function<void(const T&, ScriptRecord&)> capture)
		{
			static_assert(std::is_base_of_v<NativeScript, T>);
			if (name.empty() || !validate || !create || !capture || m_entries.count(name)) return false;
			for (const auto& entry : m_entries)
				if (entry.second.type == std::type_index(typeid(T))) return false;
			m_entries.emplace(name, Entry{ typeid(T), std::move(validate),
				[create](Entity entity, const ScriptRecord& record) { return create(entity, record) != nullptr; },
				[capture](const NativeScript& script, ScriptRecord& record) { capture(static_cast<const T&>(script), record); } });
			return true;
		}

		bool Validate(const EntityRecord& entity, std::string& error) const;
		bool Instantiate(Entity entity, const ScriptRecord& record, std::string& error) const;
		bool Capture(const NativeScriptComponent& component, std::vector<ScriptRecord>& records, std::string& error) const;

	private:
		struct Entry
		{
			std::type_index type;
			Validator validate;
			std::function<bool(Entity, const ScriptRecord&)> create;
			std::function<void(const NativeScript&, ScriptRecord&)> capture;
		};
		std::map<std::string, Entry> m_entries;
	};
}
