#pragma once

#include <filesystem>

#include "Macros.h"

#include <unordered_map>

namespace Daedalus
{
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
		
		static bool IsEmpty() { return GetInstance().m_keyboard_binds.empty(); }
		static int GetKeyboardBind(const std::string& bind_name);
		static void SetKeyboardBind(const std::string& bind_name, int button_code);
		
	private:
		static KeybindConfig& GetInstance();
		static KeybindConfig* s_instance; // Non-owning; Engine owns the configuration.
		std::unordered_map<std::string, int> m_keyboard_binds;
	};
}