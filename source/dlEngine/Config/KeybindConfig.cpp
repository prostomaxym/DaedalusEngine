#include "dlpch.h"

#include "KeybindConfig.h"
#include <SimpleIni.h>

using namespace Daedalus;

namespace
{
	constexpr auto KeyboardSectionName = "Keyboard";
}

std::unordered_map<std::string, int> KeybindConfig::s_keyboard_binds = std::unordered_map<std::string, int>();

void KeybindConfig::Load(const std::filesystem::path& path)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	SI_Error rc = ini.LoadFile(path.string().c_str());
	if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to load keybind config from " + path.string());
		return;
	}
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loaded keybind config from " + path.string());
	}

	std::list<CSimpleIniA::Entry> binds;

	ini.GetAllKeys(KeyboardSectionName, binds);
	for (const auto& bind : binds)
	{
		const auto bind_str = ini.GetValue(KeyboardSectionName, bind.pItem);
		s_keyboard_binds[bind.pItem] = std::stoi(bind_str);
	}
}

void KeybindConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	for (const auto& [key, code] : s_keyboard_binds)
	{
		ini.SetValue(KeyboardSectionName, key.c_str(), std::to_string(code).c_str());
	}

	SI_Error rc = ini.SaveFile(path.string().c_str());
	if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to save keybind config to " + path.string() + "\n");
	}
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Saved keybind config to " + path.string());
	}
}

int KeybindConfig::GetKeyboardBind(const std::string& bind_name)
{
	const auto it = s_keyboard_binds.find(bind_name);
	DL_ASSERT(it != s_keyboard_binds.end(), Log::Categories::Application, bind_name + " bind does not exist");

	return it->second;
}
void KeybindConfig::SetKeyboardBind(const std::string& bind_name, int button_code)
{
	s_keyboard_binds[bind_name] = button_code;
}