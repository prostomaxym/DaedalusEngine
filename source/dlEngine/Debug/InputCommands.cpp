#include "dlpch.h"
#include "InputCommands.h"
#include "Config/KeybindConfig.h"
#include <algorithm>
#include <locale>
#include <sstream>

namespace Daedalus
{
	namespace
	{
		constexpr auto ActionUsage = "Usage: bind_action <name> <keyboard-code|-1> <gamepad-button|-1>";
		constexpr auto AxisUsage = "Usage: bind_axis <name> <negative-action|-> <positive-action|-> <gamepad-axis|-1> <scale> <deadzone>";

		bool AtEnd(std::istringstream& input)
		{
			input >> std::ws;
			return input.eof();
		}
	}

	std::vector<std::string> InputCommands::Complete(std::string_view command, std::string_view prefix)
	{
		std::vector<std::string> names;
		if (command == "actions" || command == "bind_action")
			for (const auto& [name, binding] : KeybindConfig::GetActionBinds())
				if (name.compare(0, prefix.size(), prefix) == 0) names.push_back(name);
		if (command == "axes" || command == "bind_axis")
			for (const auto& [name, binding] : KeybindConfig::GetAxisBinds())
				if (name.compare(0, prefix.size(), prefix) == 0) names.push_back(name);
		std::sort(names.begin(), names.end());
		return names;
	}

	bool InputCommands::Execute(std::string_view command, std::vector<std::string>& output)
	{
		std::istringstream input{ std::string(command) };
		input.imbue(std::locale::classic());
		std::string verb;
		input >> verb;
		if (verb == "input_help")
		{
			if (!AtEnd(input)) output.emplace_back("Usage: input_help");
			else
			{
				output.emplace_back("actions [prefix]; axes [prefix]; edits persist on engine shutdown.");
				output.emplace_back(ActionUsage);
				output.emplace_back(AxisUsage);
				output.emplace_back("Codes use GLFW/DL constants; -1 unbinds. Gamepad buttons: 0..14; axes: 0 LeftX, 1 LeftY, 2 RightX, 3 RightY, 4 LeftTrigger, 5 RightTrigger.");
				output.emplace_back("Names: 1..64 letters/digits/underscore/dot, case-sensitive. Axis actions must exist; '-' omits a direction. Scale [-1,1] affects analog only; deadzone [0,1). Triggers normalize to [0,1]; combined axes clamp to [-1,1].");
			}
			return true;
		}
		if (verb == "actions" || verb == "axes")
		{
			std::string prefix;
			if (!AtEnd(input)) input >> prefix;
			if (!AtEnd(input))
			{
				output.emplace_back("Usage: " + verb + " [prefix]");
				return true;
			}
			const auto names = Complete(verb, prefix);
			for (const auto& name : names)
			{
				std::ostringstream line;
				line.imbue(std::locale::classic());
				line << name;
				if (verb == "actions")
				{
					const auto& binding = *KeybindConfig::FindActionBind(name);
					line << " keyboard=" << binding.keyboard << " gamepad=" << binding.gamepad_button;
				}
				else
				{
					const auto& binding = *KeybindConfig::FindAxisBind(name);
					line << " negative=" << (binding.negative_action.empty() ? "-" : binding.negative_action)
						<< " positive=" << (binding.positive_action.empty() ? "-" : binding.positive_action)
						<< " gamepad=" << static_cast<int>(binding.gamepad_axis) << " scale=" << binding.scale << " deadzone=" << binding.deadzone;
				}
				output.push_back(line.str());
			}
			if (names.empty()) output.emplace_back("No matching " + verb);
			return true;
		}
		if (verb == "bind_action")
		{
			std::string name;
			InputActionBinding binding;
			if (!(input >> name >> binding.keyboard >> binding.gamepad_button) || !AtEnd(input)) output.emplace_back(ActionUsage);
			else if (!KeybindConfig::SetActionBind(name, binding)) output.emplace_back("Invalid action binding; no changes made");
			else output.emplace_back("Bound action " + name + "; effective on next input update");
			return true;
		}
		if (verb == "bind_axis")
		{
			std::string name;
			InputAxisBinding binding;
			int axis = -1;
			if (!(input >> name >> binding.negative_action >> binding.positive_action >> axis >> binding.scale >> binding.deadzone) || !AtEnd(input))
				output.emplace_back(AxisUsage);
			else
			{
				if (binding.negative_action == "-") binding.negative_action.clear();
				if (binding.positive_action == "-") binding.positive_action.clear();
				binding.gamepad_axis = static_cast<GamepadAxis>(axis);
				if (!KeybindConfig::SetAxisBind(name, binding)) output.emplace_back("Invalid axis binding or unknown action; no changes made");
				else output.emplace_back("Bound axis " + name);
			}
			return true;
		}
		return false;
	}
}
