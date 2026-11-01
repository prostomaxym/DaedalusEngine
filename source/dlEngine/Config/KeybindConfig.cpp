#include "dlpch.h"

#include "KeybindConfig.h"
#include <SimpleIni.h>
#include "Utils/KeyCodes.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

using namespace Daedalus;

namespace
{
	constexpr auto KeyboardSectionName = "Keyboard";
	constexpr auto GamepadSectionName = "Gamepad";
	constexpr auto AxisSectionName = "Axes";

	template<typename... Values>
	bool Parse(const char* text, Values&... values)
	{
		if (!text) return false;
		std::istringstream input(text);
		input.imbue(std::locale::classic());
		if (!(input >> ... >> values)) return false;
		input >> std::ws;
		return input.eof();
	}
}

KeybindConfig* KeybindConfig::s_instance = nullptr;

KeybindConfig::KeybindConfig()
{
	DL_ASSERT(!s_instance, Log::Categories::Application, "Only one keybind configuration may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

KeybindConfig::~KeybindConfig() { s_instance = nullptr; }

KeybindConfig& KeybindConfig::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::Application, "KeybindConfig used outside its lifetime");
	if (!s_instance) std::abort();
	return *s_instance;
}

void KeybindConfig::Load(const std::filesystem::path& path)
{
	CSimpleIniCaseA ini;
	ini.SetUnicode();

	SI_Error rc = ini.LoadFile(path.string().c_str());
	if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to load keybind config from {}", path.string());
		return;
	}
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Loaded keybind config from {}", path.string());
	}

	for (const auto* section : { KeyboardSectionName, GamepadSectionName })
	{
		std::list<CSimpleIniCaseA::Entry> binds;
		ini.GetAllKeys(section, binds);
		for (const auto& bind : binds)
		{
			int code = -1;
			InputActionBinding binding;
			if (const auto* existing = FindActionBind(bind.pItem)) binding = *existing;
			const bool parsed = Parse(ini.GetValue(section, bind.pItem), code);
			if (section == KeyboardSectionName) binding.keyboard = code;
			else binding.gamepad_button = code;
			if (!parsed || !SetActionBind(bind.pItem, binding))
				Log::Write(Log::Levels::Warn, Log::Categories::Application, "Invalid {} binding '{}'", section, bind.pItem);
		}
	}
	std::list<CSimpleIniCaseA::Entry> axes;
	ini.GetAllKeys(AxisSectionName, axes);
	for (const auto& axis : axes)
	{
		InputAxisBinding binding;
		int code = -1;
		const bool parsed = Parse(ini.GetValue(AxisSectionName, axis.pItem), binding.negative_action,
			binding.positive_action, code, binding.scale, binding.deadzone);
		if (binding.negative_action == "-") binding.negative_action.clear();
		if (binding.positive_action == "-") binding.positive_action.clear();
		binding.gamepad_axis = static_cast<GamepadAxis>(code);
		if (!parsed || !SetAxisBind(axis.pItem, binding))
			Log::Write(Log::Levels::Warn, Log::Categories::Application, "Invalid axis binding '{}'", axis.pItem);
	}
}

void KeybindConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniCaseA ini;
	ini.SetUnicode();

	for (const auto& [key, binding] : GetInstance().m_action_binds)
	{
		ini.SetValue(KeyboardSectionName, key.c_str(), std::to_string(binding.keyboard).c_str());
		ini.SetValue(GamepadSectionName, key.c_str(), std::to_string(binding.gamepad_button).c_str());
	}
	for (const auto& [key, binding] : GetInstance().m_axis_binds)
	{
		std::ostringstream value;
		value.imbue(std::locale::classic());
		value << (binding.negative_action.empty() ? "-" : binding.negative_action) << ' '
			<< (binding.positive_action.empty() ? "-" : binding.positive_action) << ' '
			<< static_cast<int>(binding.gamepad_axis) << ' ' << std::setprecision(std::numeric_limits<float>::max_digits10)
			<< binding.scale << ' ' << binding.deadzone;
		ini.SetValue(AxisSectionName, key.c_str(), value.str().c_str());
	}

	SI_Error rc = ini.SaveFile(path.string().c_str());
	if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to save keybind config to {}\n", path.string());
	}
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Saved keybind config to {}", path.string());
	}
}

int KeybindConfig::GetKeyboardBind(const std::string& bind_name)
{
	const auto* binding = FindActionBind(bind_name);
	DL_ASSERT(binding, Log::Categories::Application, bind_name + " bind does not exist");
	return binding ? binding->keyboard : -1;
}
void KeybindConfig::SetKeyboardBind(const std::string& bind_name, int button_code)
{
	InputActionBinding binding;
	if (const auto* existing = FindActionBind(bind_name)) binding = *existing;
	binding.keyboard = button_code;
	if (!SetActionBind(bind_name, binding))
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Invalid keyboard binding '{}'", bind_name);
}

std::unordered_map<std::string, int> KeybindConfig::GetKeyboardBinds()
{
	std::unordered_map<std::string, int> result;
	for (const auto& [name, binding] : GetActionBinds()) result.emplace(name, binding.keyboard);
	return result;
}

const InputActionBinding* KeybindConfig::FindActionBind(const std::string& bind_name)
{
	const auto& binds = GetActionBinds();
	const auto it = binds.find(bind_name);
	return it == binds.end() ? nullptr : &it->second;
}

const InputAxisBinding* KeybindConfig::FindAxisBind(const std::string& bind_name)
{
	const auto& binds = GetAxisBinds();
	const auto it = binds.find(bind_name);
	return it == binds.end() ? nullptr : &it->second;
}

bool KeybindConfig::IsValidBindName(std::string_view name)
{
	return !name.empty() && name.size() <= 64 && std::all_of(name.begin(), name.end(), [](char character)
	{
		return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') || character == '_' || character == '.';
	});
}

bool KeybindConfig::IsValidKeyboardCode(int code)
{
	return code == -1 || code == DL_KEY_SPACE || code == DL_KEY_APOSTROPHE || (code >= DL_KEY_COMMA && code <= DL_KEY_9) ||
		code == DL_KEY_SEMICOLON || code == DL_KEY_EQUAL || (code >= DL_KEY_A && code <= DL_KEY_RIGHT_BRACKET) ||
		code == DL_KEY_GRAVE_ACCENT || code == DL_KEY_WORLD_1 || code == DL_KEY_WORLD_2 ||
		(code >= DL_KEY_ESCAPE && code <= DL_KEY_END) || (code >= DL_KEY_CAPS_LOCK && code <= DL_KEY_PAUSE) ||
		(code >= DL_KEY_F1 && code <= DL_KEY_F25) || (code >= DL_KEY_KP_0 && code <= DL_KEY_KP_EQUAL) ||
		(code >= DL_KEY_LEFT_SHIFT && code <= DL_KEY_MENU);
}

bool KeybindConfig::SetActionBind(const std::string& bind_name, InputActionBinding binding)
{
	if (!IsValidBindName(bind_name) || !IsValidKeyboardCode(binding.keyboard) ||
		binding.gamepad_button < -1 || binding.gamepad_button > DL_GAMEPAD_BUTTON_LAST) return false;
	auto& config = GetInstance();
	if (const auto* existing = FindActionBind(bind_name))
		if (existing->keyboard == binding.keyboard && existing->gamepad_button == binding.gamepad_button) return true;
	config.m_action_binds[bind_name] = binding;
	++config.m_revision;
	return true;
}

bool KeybindConfig::SetAxisBind(const std::string& bind_name, const InputAxisBinding& binding)
{
	const int code = static_cast<int>(binding.gamepad_axis);
	if (!IsValidBindName(bind_name) || code < -1 || code > DL_GAMEPAD_AXIS_LAST ||
		!std::isfinite(binding.scale) || std::abs(binding.scale) > 1.f ||
		!std::isfinite(binding.deadzone) || binding.deadzone < 0.f || binding.deadzone >= 1.f ||
		(!binding.negative_action.empty() && !FindActionBind(binding.negative_action)) ||
		(!binding.positive_action.empty() && !FindActionBind(binding.positive_action))) return false;
	auto& config = GetInstance();
	if (const auto* existing = FindAxisBind(bind_name))
		if (existing->negative_action == binding.negative_action && existing->positive_action == binding.positive_action &&
			existing->gamepad_axis == binding.gamepad_axis && existing->scale == binding.scale && existing->deadzone == binding.deadzone) return true;
	config.m_axis_binds[bind_name] = binding;
	++config.m_revision;
	return true;
}
