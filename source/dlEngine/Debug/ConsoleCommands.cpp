#include "dlpch.h"
#include "ConsoleCommands.h"
#include <array>
#include <sstream>

namespace Daedalus
{
	namespace
	{
		struct CommandInfo
		{
			std::string_view name;
			std::string_view syntax;
			std::string_view description;
		};

		constexpr std::array<CommandInfo, 12> Commands = {{
			{ "actions", "actions [prefix]", "List live action bindings." },
			{ "axes", "axes [prefix]", "List live axis bindings." },
			{ "bind_action", "bind_action <name> <keyboard-code|-1> <gamepad-button|-1>", "Create or rebind an action; -1 unbinds a device." },
			{ "bind_axis", "bind_axis <name> <negative-action|-> <positive-action|-> <gamepad-axis|-1> <scale> <deadzone>", "Create or rebind an axis; '-' omits a direction." },
			{ "clear", "clear", "Clear console output and command history." },
			{ "cvars", "cvars [prefix]", "List configuration variables, values and runtime/read-only status." },
			{ "get", "get <name>", "Read a configuration variable." },
			{ "help", "help", "Show all supported commands." },
			{ "input_help", "input_help", "Show input binding syntax, codes and persistence rules." },
			{ "quit", "quit", "Close the application." },
			{ "set", "set <name> <value>", "Change a writable configuration variable." },
			{ "version", "version", "Show application and build information." }
		}};
	}

	bool ConsoleCommands::ExecuteHelp(std::string_view command, std::vector<std::string>& output)
	{
		std::istringstream input{ std::string(command) };
		std::string verb, extra;
		input >> verb;
		if (verb != "help") return false;
		if (input >> extra)
		{
			output.emplace_back("Usage: help");
			return true;
		}
		output.emplace_back("Supported commands ([] optional, <> required):");
		for (const auto& entry : Commands)
			output.push_back(std::string(entry.syntax) + " - " + std::string(entry.description));
		return true;
	}

	std::vector<std::string> ConsoleCommands::Complete(std::string_view prefix)
	{
		std::vector<std::string> result;
		for (const auto& entry : Commands)
			if (entry.name.substr(0, prefix.size()) == prefix) result.emplace_back(entry.name);
		return result;
	}
}
