#pragma once

#include "ECS/NativeScript.h"
#include "LuaRuntime.h"

namespace Daedalus
{
	class ScriptRegistry;

	class DAEDALUS_API LuaScript final : public NativeScript
	{
	public:
		LuaScript(Entity entity, VirtualFileSystem files, ScriptRecord record);
		~LuaScript() override;
		LuaRuntime& GetRuntime() { return m_runtime; }
		const ScriptRecord& GetRecord() const { return m_record; }
		[[nodiscard]] static bool Register(ScriptRegistry& registry, VirtualFileSystem files);

	private:
		void OnUpdate(DeltaTime dt) override;
		void OnEvent(const Event& event) override;
		void OnRuntimeStop() override;
		ScriptRecord m_record;
		LuaRuntime m_runtime;
	};
}
