#include "dlpch.h"
#include "ScriptRegistry.h"

#include <algorithm>
#include <set>

using namespace Daedalus;

bool ScriptRegistry::Validate(const EntityRecord& entity, std::string& error) const
{
	std::set<std::type_index> types;
	for (const auto& record : entity.scripts)
	{
		const auto it = m_entries.find(record.type);
		if (it == m_entries.end())
		{
			error = "Entity " + entity.id + ": unregistered script " + record.type;
			return false;
		}
		if (!types.insert(it->second.type).second || !it->second.validate(record, entity, error))
		{
			error = "Entity " + entity.id + ", script " + record.type + ": " + (error.empty() ? "duplicate script type" : error);
			return false;
		}
	}
	return true;
}

bool ScriptRegistry::Instantiate(Entity entity, const ScriptRecord& record, std::string& error) const
{
	const auto it = m_entries.find(record.type);
	if (it == m_entries.end() || !it->second.create(entity, record))
	{
		error = "Failed to construct script " + record.type + " on entity " + entity.GetName();
		return false;
	}
	const auto& scripts = entity.GetComponent<NativeScriptComponent>().native_scripts;
	if (!scripts.count(it->second.type))
	{
		error = "Script factory did not attach the registered type: " + record.type;
		return false;
	}
	return true;
}

bool ScriptRegistry::Capture(const NativeScriptComponent& component, std::vector<ScriptRecord>& records, std::string& error) const
{
	for (const auto& script : component.native_scripts)
	{
		const auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto& entry) { return entry.second.type == script.first; });
		if (it == m_entries.end() || !script.second)
		{
			error = "Cannot save an unregistered or null native script";
			return false;
		}
		ScriptRecord record;
		record.type = it->first;
		it->second.capture(*script.second, record);
		records.push_back(std::move(record));
	}
	std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) { return a.type < b.type; });
	return true;
}
