#pragma once

#include <filesystem>

#include "Daedalus/Core/Core.h"

#include <unordered_map>

namespace Daedalus
{
	class DAEDALUS_API KeybindConfig
	{
	public:
		static void Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path);
		
		static bool IsEmpty() { return s_keyboard_binds.empty(); }
		static int GetKeyboardBind(const std::string& bind_name);
		static void SetKeyboardBind(const std::string& bind_name, int button_code);
		
	private:
		static std::unordered_map<std::string, int> s_keyboard_binds;
	};
}