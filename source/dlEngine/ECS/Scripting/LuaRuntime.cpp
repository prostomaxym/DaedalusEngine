#include "LuaRuntime.h"
#include "ECS/Entity.h"
#include "Core/Input.h"
#include "Debug/Log.h"
#include "Events/GameEvent.h"
#include "Events/GamepadEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/PhysicsEvent.h"
#include <sol/sol.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace Daedalus
{
	namespace
	{
		constexpr size_t MaxProperties = 128;
		constexpr size_t MaxStringBytes = 4096;

		bool Finite(const glm::vec3& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z)
				&& glm::all(glm::lessThanEqual(glm::abs(value), glm::vec3(1.e8f)));
		}

		UUID ParseUUID(const std::string& text)
		{
			uint64_t value = 0;
			const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
			return UUID(result.ec == std::errc{} && result.ptr == text.data() + text.size() ? value : 0);
		}

		std::string Text(const sol::object& value)
		{
			if (value.get_type() != sol::type::string) return {};
			auto text = value.as<std::string>();
			return text.size() <= MaxStringBytes ? text : std::string{};
		}
	}

	struct LuaRuntime::State
	{
		struct Memory
		{
			size_t used{ 0 };
			bool enforce{ false };
		} memory;

		static void* Allocate(void* context, void* pointer, size_t old_size, size_t new_size)
		{
			auto& memory = *static_cast<Memory*>(context);
			if (!pointer) old_size = 0;
			if (!new_size)
			{
				std::free(pointer);
				memory.used -= old_size;
				return nullptr;
			}
			if (memory.enforce && new_size > old_size
				&& new_size - old_size > MaxMemoryBytes - (std::min)(memory.used, MaxMemoryBytes)) return nullptr;
			auto* result = std::realloc(pointer, new_size);
			if (result) memory.used = memory.used - old_size + new_size;
			return result;
		}

		static void Hook(lua_State* state, lua_Debug*)
		{
			auto& self = **static_cast<State**>(lua_getextraspace(state));
			self.instructions += 1000;
			if (self.instructions >= MaxInstructions) luaL_error(state, "Lua instruction budget exceeded");
		}

		sol::state lua;
		int instructions{ 0 };

		State() : lua(sol::default_at_panic, Allocate, &memory)
		{
			*static_cast<State**>(lua_getextraspace(lua.lua_state())) = this;
			#if LUA_VERSION_NUM >= 505
			luaL_openselectedlibs(lua.lua_state(), LUA_GLIBK | LUA_MATHLIBK | LUA_TABLIBK | LUA_STRLIBK | LUA_UTF8LIBK, 0);
			#else
			lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::utf8);
			#endif
			// Protected calls/coroutines could repeatedly swallow the instruction hook's error.
			for (const char* name : { "dofile", "loadfile", "load", "require", "collectgarbage", "print",
				"getmetatable", "setmetatable", "rawget", "rawset", "pcall", "xpcall" }) lua[name] = sol::nil;
			lua["string"]["dump"] = sol::nil;
			lua.new_usertype<glm::vec3>("Vector3", sol::no_constructor,
				"x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);
			lua.set_function("vec3", [](sol::object x, sol::object y, sol::object z) -> sol::optional<glm::vec3>
			{
				if (x.get_type() != sol::type::number || y.get_type() != sol::type::number || z.get_type() != sol::type::number)
					return sol::nullopt;
				return glm::vec3(x.as<float>(), y.as<float>(), z.as<float>());
			});
		}

		void Arm()
		{
			instructions = 0;
			memory.enforce = true;
			lua_sethook(lua.lua_state(), Hook, LUA_MASKCOUNT, 1000);
		}

		void Disarm()
		{
			lua_sethook(lua.lua_state(), nullptr, 0, 0);
			memory.enforce = false;
		}

		template<typename... Args>
		bool Call(const char* name, std::string& error, Args&&... args)
		{
			sol::object object = lua[name];
			if (!object.valid()) return true;
			if (object.get_type() != sol::type::function)
			{
				error = std::string(name) + " must be a function";
				return false;
			}
			sol::protected_function function = object;
			Arm();
			auto result = function(std::forward<Args>(args)...);
			Disarm();
			if (result.valid()) return true;
			error = result.template get<sol::error>().what();
			return false;
		}

		void SetProperties(const char* name, const Properties& properties)
		{
			auto table = lua.create_named_table(name);
			for (const auto& [key, value] : properties)
				std::visit([&](const auto& typed) { table[key] = typed; }, value);
		}

		bool ReadState(Properties& properties, std::string& error) const
		{
			sol::object object = lua["state"];
			if (object.get_type() != sol::type::table)
			{
				error = "state must remain a flat property table";
				return false;
			}
			sol::table table = object;
			for (const auto& [key, value] : table)
			{
				if (key.get_type() != sol::type::string || properties.size() >= MaxProperties)
				{
					error = "state requires at most 128 string-keyed properties";
					return false;
				}
				const auto name = key.as<std::string>();
				if (value.get_type() == sol::type::boolean) properties[name] = value.as<bool>();
				else if (value.get_type() == sol::type::number) properties[name] = value.as<double>();
				else if (value.get_type() == sol::type::string) properties[name] = value.as<std::string>();
				else if (value.is<glm::vec3>()) properties[name] = value.as<glm::vec3>();
				else
				{
					error = "state values must be bool, number, string or vec3";
					return false;
				}
			}
			return ValidateProperties(properties, error);
		}

		bool Load(const std::string& source, const std::string& path, const Properties& properties,
			const Properties& state, bool reload, std::string& error)
		{
			SetProperties("properties", properties);
			SetProperties("state", state);
			Arm();
			auto result = lua.safe_script(source, sol::script_pass_on_error, "@" + path, sol::load_mode::text);
			Disarm();
			if (!result.valid())
			{
				error = result.get<sol::error>().what();
				return false;
			}
			if (reload && !Call("on_reload", error)) return false;
			for (const char* name : { "on_start", "on_update", "on_event", "on_stop", "on_reload" })
			{
				sol::object callback = lua[name];
				if (callback.valid() && callback.get_type() != sol::type::function)
				{
					error = std::string(name) + " must be a function";
					return false;
				}
			}
			Properties checked;
			return ReadState(checked, error);
		}

		void Bind(Scene& scene, UUID self)
		{
			auto resolve = [&scene](const sol::object& id) { return scene.GetEntityByUUID(ParseUUID(Text(id))); };
			auto ecs = lua.create_named_table("ecs");
			ecs["self"] = std::to_string(static_cast<uint64_t>(self));
			ecs.set_function("valid", [resolve](sol::object id) { return static_cast<bool>(resolve(id)); });
			ecs.set_function("find", [&scene](sol::object value)
			{
				const auto name = Text(value);
				auto entity = !name.empty() ? scene.FindEntityByName(name) : Entity{};
				return entity ? std::to_string(static_cast<uint64_t>(entity.GetUUID())) : std::string{};
			});
			ecs.set_function("name", [resolve](sol::object id)
			{
				auto entity = resolve(id);
				return entity ? entity.GetName() : std::string{};
			});
			ecs.set_function("link", [&scene](sol::object id, sol::object name)
			{
				return std::to_string(static_cast<uint64_t>(scene.GetEntityLink(ParseUUID(Text(id)), Text(name))));
			});
			auto bind_transform = [&](const char* get, const char* set, glm::vec3 TransformComponent::* member)
			{
				ecs.set_function(get, [resolve, member](sol::object id) -> sol::optional<glm::vec3>
				{
					auto entity = resolve(id);
					if (!entity || !entity.HasComponent<TransformComponent>()) return sol::nullopt;
					return entity.GetComponent<TransformComponent>().*member;
				});
				ecs.set_function(set, [resolve, member](sol::object id, sol::object object)
				{
					auto entity = resolve(id);
					if (!entity || !entity.HasComponent<TransformComponent>()
						|| object.get_type() != sol::type::userdata || !object.is<glm::vec3>()) return false;
					const auto value = object.as<glm::vec3>();
					if (!Finite(value)) return false;
					entity.GetComponent<TransformComponent>().*member = value;
					return true;
				});
			};
			bind_transform("position", "set_position", &TransformComponent::translation);
			bind_transform("rotation", "set_rotation", &TransformComponent::rotation);
			bind_transform("scale", "set_scale", &TransformComponent::scale);

			auto input = lua.create_named_table("input");
			auto bind_action = [&](const char* name, bool (*query)(const std::string&))
			{
				input.set_function(name, [query](sol::object value)
				{
					const auto action = Text(value);
					return !action.empty() && action.size() <= 128 && Input::IsAvailable() && query(action);
				});
			};
			bind_action("down", Input::IsActionDown);
			bind_action("pressed", Input::IsActionPressed);
			bind_action("held", Input::IsActionHold);
			bind_action("released", Input::IsActionReleased);
			input.set_function("axis", [](sol::object value)
			{
				const auto name = Text(value);
				return !name.empty() && name.size() <= 128 && Input::IsAvailable() ? Input::GetAxis(name) : 0.f;
			});
			auto events = lua.create_named_table("events");
			events.set_function("emit", [&scene, self](sol::object topic, sol::object data)
			{
				if (topic.get_type() != sol::type::string || data.get_type() != sol::type::string) return false;
				const auto name = Text(topic);
				const auto payload = data.as<std::string>();
				if (name.empty() || name.size() > 128 || payload.size() > MaxStringBytes) return false;
				return scene.QueueScriptEvent(std::make_unique<GameEvent>(name, payload, static_cast<uint64_t>(self)));
			});
		}
	};

	LuaRuntime::LuaRuntime(Scene& scene, UUID entity, VirtualFileSystem files, std::string path, Properties properties)
		: m_scene(scene), m_entity(entity), m_files(std::move(files)), m_path(std::move(path)), m_properties(std::move(properties))
	{
	}

	LuaRuntime::~LuaRuntime() = default;

	bool LuaRuntime::CheckThread() const
	{
		return m_owner == std::this_thread::get_id();
	}

	void LuaRuntime::Report(std::string error)
	{
		error.resize((std::min)(error.size(), MaxStringBytes));
		if (error != m_error)
			Log::Write(Log::Levels::Error, Log::Categories::ECS, "Lua {}: {}", m_path, error);
		m_error = std::move(error);
	}

	bool LuaRuntime::ValidateProperties(const Properties& properties, std::string& error)
	{
		if (properties.size() > MaxProperties)
		{
			error = "Lua permits at most 128 properties";
			return false;
		}
		for (const auto& [name, value] : properties)
		{
			const bool valid = !name.empty() && name.size() <= 128 && std::visit([](const auto& typed)
			{
				using T = std::decay_t<decltype(typed)>;
				if constexpr (std::is_same_v<T, double>) return std::isfinite(typed);
				else if constexpr (std::is_same_v<T, std::string>) return typed.size() <= MaxStringBytes;
				else if constexpr (std::is_same_v<T, glm::vec3>) return Finite(typed);
				else return true;
			}, value);
			if (!valid)
			{
				error = "Invalid Lua property: " + name.substr(0, 128);
				return false;
			}
		}
		return true;
	}

	bool LuaRuntime::ValidateSource(const VirtualFileSystem& files, const std::string& path, const Properties& properties, std::string& error)
	{
		if (!ValidateProperties(properties, error)) return false;
		const auto normalized = VirtualFileSystem::Normalize(path);
		if (!normalized || *normalized != path || !path.ends_with(".lua"))
		{
			error = "Lua asset requires a normalized mounted .lua path";
			return false;
		}
		std::string source;
		if (!files.Read(path, source, error, MaxSourceBytes)) return false;
		State candidate;
		return candidate.Load(source, path, properties, properties, false, error);
	}

	bool LuaRuntime::Reload(bool force)
	{
		if (!CheckThread()) return false;
		m_attempted = true;
		std::string error, source;
		const auto normalized = VirtualFileSystem::Normalize(m_path);
		if (!normalized || *normalized != m_path || !m_path.ends_with(".lua"))
		{
			Report("Lua asset requires a normalized mounted .lua path");
			return false;
		}
		if (!ValidateProperties(m_properties, error) || !m_files.Read(m_path, source, error, MaxSourceBytes))
		{
			Report(error);
			return false;
		}
		if (!force && m_last_attempt && source == *m_last_attempt) return IsReady();
		m_last_attempt = source;
		Properties state = m_properties;
		if (m_state)
		{
			state.clear();
			if (!m_state->ReadState(state, error))
			{
				if (!m_faulted)
				{
					Report(error);
					return false;
				}
				state = m_properties;
				error.clear();
			}
		}
		auto candidate = std::make_unique<State>();
		if (!candidate->Load(source, m_path, m_properties, state, m_started, error))
		{
			Report(error);
			return false;
		}
		candidate->Bind(m_scene, m_entity);
		m_state = std::move(candidate);
		m_faulted = false;
		m_error.clear();
		return true;
	}

	void LuaRuntime::Update(float seconds)
	{
		if (!CheckThread() || !std::isfinite(seconds) || seconds < 0.f) return;
		m_poll_seconds += seconds;
		if (!m_attempted || m_poll_seconds >= 0.5f)
		{
			m_poll_seconds = 0.f;
			(void)Reload(false);
		}
		if (!IsReady()) return;
		std::string error;
		if (!m_started)
		{
			m_started = true;
			if (!m_state->Call("on_start", error))
			{
				m_faulted = true;
				Report(error);
				return;
			}
		}
		if (!m_state->Call("on_update", error, seconds))
		{
			m_faulted = true;
			Report(error);
		}
	}

	void LuaRuntime::OnEvent(const Event& event)
	{
		if (!CheckThread() || !IsReady() || !m_started) return;
		auto data = m_state->lua.create_table();
		data["type"] = event.GetNameC();
		if (const auto* key = dynamic_cast<const KeyEvent*>(&event)) data["key"] = key->GetKeyCode();
		if (const auto* key = dynamic_cast<const KeyPressedEvent*>(&event)) data["repeat_count"] = key->GetRepeatCount();
		if (const auto* button = dynamic_cast<const MouseButtonEvent*>(&event)) data["button"] = button->GetMouseButton();
		if (const auto* mouse = dynamic_cast<const MouseMovedEvent*>(&event))
		{
			data["x"] = mouse->GetX();
			data["y"] = mouse->GetY();
		}
		if (const auto* scroll = dynamic_cast<const MouseScrolledEvent*>(&event))
		{
			data["x"] = scroll->GetOffsetX();
			data["y"] = scroll->GetOffsetY();
		}
		if (const auto* button = dynamic_cast<const GpadButtonEvent*>(&event)) data["button"] = button->GetKeyCode();
		if (const auto* button = dynamic_cast<const GpadPressedEvent*>(&event)) data["repeat_count"] = button->GetRepeatCount();
		if (const auto* analog = dynamic_cast<const GpadAnalogEvent*>(&event)) data["axis"] = analog->GetKeyCode();
		if (const auto* stick = dynamic_cast<const GpadStickEvent*>(&event)) data["value"] = stick->GetPercentagePushed();
		if (const auto* trigger = dynamic_cast<const GpadTriggerEvent*>(&event)) data["value"] = trigger->GetPercentagePushed();
		if (const auto* physics = dynamic_cast<const ScenePhysicsEvent*>(&event))
		{
			data["scene"] = std::to_string(physics->GetScene());
			data["first"] = std::to_string(physics->GetFirstEntity());
			data["second"] = std::to_string(physics->GetSecondEntity());
			data["phase"] = static_cast<int>(physics->GetContact().phase);
			data["position"] = physics->GetContact().position;
			data["normal"] = physics->GetContact().normal;
			data["impulse"] = physics->GetContact().impulse;
			data["trigger"] = physics->GetContact().trigger;
		}
		if (const auto* game = dynamic_cast<const GameEvent*>(&event))
		{
			data["name"] = game->GetTopic();
			data["payload"] = game->GetPayload();
			data["sender"] = std::to_string(game->GetSender());
		}
		std::string error;
		if (!m_state->Call("on_event", error, data))
		{
			m_faulted = true;
			Report(error);
		}
	}

	void LuaRuntime::Stop()
	{
		if (!CheckThread()) return;
		std::string error;
		if (IsReady() && m_started && !m_state->Call("on_stop", error)) Report(error);
		m_state.reset();
		m_started = false;
		m_attempted = false;
		m_faulted = false;
		m_poll_seconds = 0.f;
		m_last_attempt.reset();
	}

	bool LuaRuntime::IsReady() const
	{
		return m_state && !m_faulted;
	}

	LuaRuntime::Properties LuaRuntime::GetState() const
	{
		Properties result;
		std::string error;
		if (CheckThread() && m_state && !m_state->ReadState(result, error)) result.clear();
		return result;
	}
}
