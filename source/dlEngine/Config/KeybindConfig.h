#pragma once

#include <filesystem>

#include "Macros.h"

#include <unordered_map>
#include <string>
#include <cstdint>
#include <string_view>

namespace Daedalus
{
	struct InputActionBinding
	{
		int keyboard{ -1 };
		int gamepad_button{ -1 };
	};

	enum class GamepadAxis
	{
		None = -1, LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger
	};

	struct InputAxisBinding
	{
		std::string negative_action;
		std::string positive_action;
		GamepadAxis gamepad_axis{ GamepadAxis::None };
		float scale{ 1.f };
		float deadzone{ 0.15f };
	};

	class DAEDALUS_API KeybindConfig
	{
	public:
		KeybindConfig();
		~KeybindConfig();
		KeybindConfig(const KeybindConfig&) = delete;
		KeybindConfig& operator=(const KeybindConfig&) = delete;
		KeybindConfig(KeybindConfig&&) = delete;
		KeybindConfig& operator=(KeybindConfig&&) = delete;
		static void Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path);
		
		static bool IsEmpty() { return GetInstance().m_action_binds.empty(); }
		static int GetKeyboardBind(const std::string& bind_name);
		static void SetKeyboardBind(const std::string& bind_name, int button_code);
		static std::unordered_map<std::string, int> GetKeyboardBinds();
		[[nodiscard]] static bool SetActionBind(const std::string& bind_name, InputActionBinding binding);
		[[nodiscard]] static bool SetAxisBind(const std::string& bind_name, const InputAxisBinding& binding);
		[[nodiscard]] static const InputActionBinding* FindActionBind(const std::string& bind_name);
		[[nodiscard]] static const InputAxisBinding* FindAxisBind(const std::string& bind_name);
		static const std::unordered_map<std::string, InputActionBinding>& GetActionBinds() { return GetInstance().m_action_binds; }
		static const std::unordered_map<std::string, InputAxisBinding>& GetAxisBinds() { return GetInstance().m_axis_binds; }
		static uint64_t GetRevision() { return GetInstance().m_revision; }
		[[nodiscard]] static bool IsValidBindName(std::string_view name);
		[[nodiscard]] static bool IsValidKeyboardCode(int code);
		
	private:
		static KeybindConfig& GetInstance();
		static KeybindConfig* s_instance; // Non-owning; Engine owns the configuration.
		std::unordered_map<std::string, InputActionBinding> m_action_binds;
		std::unordered_map<std::string, InputAxisBinding> m_axis_binds;
		uint64_t m_revision{ 1 };
	};
}