#include "LuaScript.h"
#include "ECS/Serialization/ScriptRegistry.h"
#include <variant>

namespace Daedalus
{
	namespace
	{
		LuaRuntime::Properties Parameters(const ScriptRecord& record)
		{
			auto result = record.properties;
			result.erase("asset");
			return result;
		}

		std::string Asset(const ScriptRecord& record)
		{
			const auto found = record.properties.find("asset");
			return found != record.properties.end() && std::holds_alternative<std::string>(found->second)
				? std::get<std::string>(found->second) : std::string{};
		}
	}

	LuaScript::LuaScript(Entity entity, VirtualFileSystem files, ScriptRecord record)
		: NativeScript(entity), m_record(std::move(record)),
		m_runtime(entity.GetScene(), entity.GetUUID(), std::move(files), Asset(m_record), Parameters(m_record))
	{
	}

	LuaScript::~LuaScript()
	{
		m_runtime.Stop();
	}

	void LuaScript::OnUpdate(DeltaTime dt)
	{
		m_runtime.Update(dt.GetSeconds());
	}

	void LuaScript::OnEvent(const Event& event)
	{
		m_runtime.OnEvent(event);
	}

	void LuaScript::OnRuntimeStop()
	{
		m_runtime.Stop();
	}

	bool LuaScript::Register(ScriptRegistry& registry, VirtualFileSystem files)
	{
		return registry.Register<LuaScript>("Lua",
			[files](const ScriptRecord& record, const EntityRecord&, std::string& error)
			{
				return LuaRuntime::ValidateSource(files, Asset(record), Parameters(record), error);
			},
			[files](Entity entity, const ScriptRecord& record) -> LuaScript*
			{
				auto& component = entity.GetComponent<NativeScriptComponent>();
				if (component.ScriptExists<LuaScript>()) return nullptr;
				auto* script = component.AddScript<LuaScript>(entity, files, record);
				if (script->GetRuntime().Reload()) return script;
				component.RemoveScript<LuaScript>();
				return nullptr;
			},
			[](const LuaScript& script, ScriptRecord& record) { record.properties = script.GetRecord().properties; });
	}
}
