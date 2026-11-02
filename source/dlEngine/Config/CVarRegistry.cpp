#include "dlpch.h"
#include "CVarRegistry.h"

#include <charconv>
#include <cmath>
#include <sstream>
#include <type_traits>

namespace Daedalus
{
	namespace
	{
		std::string_view Trim(std::string_view text)
		{
			const auto first = text.find_first_not_of(" \t\r\n");
			if (first == std::string_view::npos) return {};
			return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
		}

		template<typename T>
		bool ParseNumber(std::string_view text, T& value)
		{
			if (text.empty()) return false;
#if defined(_LIBCPP_VERSION)
			if constexpr (std::is_floating_point_v<T>)
			{
				// libc++ in the Android NDK does not implement floating-point from_chars.
				std::istringstream stream{ std::string(text) };
				stream.imbue(std::locale::classic());
				T parsed{};
				if (!(stream >> parsed) || stream.peek() != std::char_traits<char>::eof()) return false;
				value = parsed;
				return true;
			}
			else
#endif
			{
				const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
				return result.ec == std::errc{} && result.ptr == text.data() + text.size();
			}
		}
	}

	bool CVarRegistry::Register(std::string name, std::string description, Getter getter, Setter setter)
	{
		if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.") != std::string::npos || !getter)
			return false;
		const auto type = getter().index();
		return m_entries.emplace(std::move(name), Entry{ std::move(description), std::move(getter), std::move(setter), type }).second;
	}

	std::string CVarRegistry::Format(const Value& value)
	{
		return std::visit([](const auto& item) -> std::string
		{
			using T = std::decay_t<decltype(item)>;
			if constexpr (std::is_same_v<T, bool>) return item ? "true" : "false";
			else if constexpr (std::is_same_v<T, std::string>) return item;
			else
			{
				char buffer[64];
				const auto result = std::to_chars(buffer, buffer + sizeof(buffer), item);
				return result.ec == std::errc{} ? std::string(buffer, result.ptr) : "<unrepresentable>";
			}
		}, value);
	}

	bool CVarRegistry::Get(std::string_view name, std::string& value, std::string& error) const
	{
		error.clear();
		value.clear();
		const auto it = m_entries.find(name);
		if (it == m_entries.end())
		{
			error = "Unknown CVar: " + std::string(name);
			return false;
		}
		value = Format(it->second.getter());
		return true;
	}

	bool CVarRegistry::Set(std::string_view name, std::string_view text, std::string& error)
	{
		error.clear();
		const auto it = m_entries.find(name);
		if (it == m_entries.end())
		{
			error = "Unknown CVar: " + std::string(name);
			return false;
		}
		const auto& entry = it->second;
		if (!entry.setter)
		{
			error = "Read-only CVar: " + std::string(name);
			return false;
		}
		text = Trim(text);
		Value value;
		bool valid = false;
		switch (entry.type)
		{
		case 0:
			valid = text == "true" || text == "false" || text == "1" || text == "0";
			value = text == "true" || text == "1";
			break;
		case 1:
		{
			int number = 0;
			valid = ParseNumber(text, number);
			value = number;
			break;
		}
		case 2:
		{
			float number = 0.f;
			valid = ParseNumber(text, number) && std::isfinite(number);
			value = number;
			break;
		}
		case 3:
			value = std::string(text);
			valid = true;
			break;
		}
		if (!valid || !entry.setter(value))
		{
			error = "Invalid value for CVar: " + std::string(name);
			return false;
		}
		return true;
	}

	std::vector<std::string> CVarRegistry::Complete(std::string_view prefix) const
	{
		std::vector<std::string> result;
		for (const auto& [name, entry] : m_entries)
			if (name.compare(0, prefix.size(), prefix) == 0) result.push_back(name);
		return result;
	}

	bool CVarRegistry::Execute(std::string_view command, std::vector<std::string>& output)
	{
		std::istringstream input{ std::string(command) };
		std::string verb, name, remainder;
		input >> verb;
		if (verb != "cvars" && verb != "get" && verb != "set") return false;
		input >> name;
		std::getline(input, remainder);
		const auto text = Trim(remainder);
		if (!text.empty() && verb != "set")
		{
			output.push_back("Usage: cvars [prefix] | get <name> | set <name> <value>");
			return true;
		}
		if (verb == "cvars")
		{
			const auto output_start = output.size();
			for (const auto& key : Complete(name))
			{
				const auto& entry = m_entries.find(key)->second;
				output.push_back(key + " = " + Format(entry.getter()) + (entry.setter ? " [runtime] " : " [read-only] ") + entry.description);
			}
			if (output.size() == output_start) output.push_back("No matching CVars");
			return true;
		}
		if (name.empty() || (verb == "set" && text.empty()))
		{
			output.push_back("Usage: get <name> | set <name> <value>");
			return true;
		}
		std::string value, error;
		if (verb == "set" && !Set(name, text, error)) output.push_back(error);
		else if (!Get(name, value, error)) output.push_back(error);
		else output.push_back(name + " = " + value);
		return true;
	}
}
