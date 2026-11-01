#include "dlpch.h"
#include "ConsoleCompletion.h"
#include "Config/CVarRegistry.h"
#include <array>

namespace Daedalus
{
	namespace
	{
		constexpr std::string_view Whitespace = " \t\r\n\v\f";
		constexpr std::array<std::string_view, 6> Commands = { "clear", "cvars", "get", "quit", "set", "version" };

		std::string_view Trim(std::string_view text)
		{
			const auto first = text.find_first_not_of(Whitespace);
			if (first == std::string_view::npos) return {};
			return text.substr(first, text.find_last_not_of(Whitespace) - first + 1);
		}
	}

	ConsoleCompletion::Matches ConsoleCompletion::Find(std::string_view input, size_t cursor, const CVarRegistry& cvars)
	{
		Matches result;
		if (cursor > input.size()) return result;
		result.offset = cursor;
		while (result.offset && Whitespace.find(input[result.offset - 1]) == std::string_view::npos)
			--result.offset;
		result.length = cursor - result.offset;
		const auto word = input.substr(result.offset, result.length);
		const auto prefix = Trim(input.substr(0, result.offset));
		if (prefix == "get" || prefix == "set" || prefix == "cvars")
			result.candidates = cvars.Complete(word);
		else if (prefix.empty())
			for (const auto command : Commands)
				if (command.substr(0, word.size()) == word) result.candidates.emplace_back(command);
		return result;
	}

	std::string ConsoleCompletion::Hint(std::string_view input, size_t cursor, bool has_selection,
		size_t buffer_capacity, const CVarRegistry& cvars)
	{
		if (cursor != input.size() || has_selection || Trim(input).empty()) return {};
		const auto matches = Find(input, cursor, cvars);
		if (matches.candidates.empty()) return {};
		const auto& completion = matches.candidates.front();
		const auto suffix_length = completion.size() - matches.length;
		if (input.size() >= buffer_capacity || suffix_length >= buffer_capacity - input.size()) return {};
		return completion.substr(matches.length);
	}
}
