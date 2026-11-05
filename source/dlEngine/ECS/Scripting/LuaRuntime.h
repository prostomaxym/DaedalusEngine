#pragma once

#include "Macros.h"
#include "ECS/UUID.h"
#include "ECS/Serialization/SceneRecord.h"
#include "Utils/VirtualFileSystem.h"
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace Daedalus
{
	class Scene;
	class Event;

	class DAEDALUS_API LuaRuntime
	{
	public:
		using Properties = std::map<std::string, ScriptProperty>;
		static constexpr size_t MaxSourceBytes = 256 * 1024;
		static constexpr size_t MaxMemoryBytes = 8 * 1024 * 1024;
		static constexpr int MaxInstructions = 100000;

		LuaRuntime(Scene& scene, UUID entity, VirtualFileSystem files, std::string path, Properties properties = {});
		~LuaRuntime();
		LuaRuntime(const LuaRuntime&) = delete;
		LuaRuntime& operator=(const LuaRuntime&) = delete;
		LuaRuntime(LuaRuntime&&) = delete;
		LuaRuntime& operator=(LuaRuntime&&) = delete;

		[[nodiscard]] bool Reload(bool force = true);
		void Update(float seconds);
		void OnEvent(const Event& event);
		void Stop();
		[[nodiscard]] bool IsReady() const;
		[[nodiscard]] const std::string& GetLastError() const { return m_error; }
		[[nodiscard]] Properties GetState() const;
		[[nodiscard]] static bool ValidateProperties(const Properties& properties, std::string& error);
		[[nodiscard]] static bool ValidateSource(const VirtualFileSystem& files, const std::string& path, const Properties& properties, std::string& error);

	private:
		struct State;
		bool CheckThread() const;
		void Report(std::string error);
		Scene& m_scene; // Non-owning; the scene owns the script and its runtime.
		UUID m_entity;
		VirtualFileSystem m_files;
		std::string m_path;
		Properties m_properties;
		std::unique_ptr<State> m_state;
		std::optional<std::string> m_last_attempt;
		std::string m_error;
		std::thread::id m_owner{ std::this_thread::get_id() };
		float m_poll_seconds{ 0.f };
		bool m_attempted{ false };
		bool m_started{ false };
		bool m_faulted{ false };
	};
}
