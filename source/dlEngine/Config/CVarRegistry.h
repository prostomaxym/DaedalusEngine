#pragma once

#include "Macros.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Daedalus
{
	// Owner-thread registry; callbacks must outlive the registry and keep a stable value type.
	class DAEDALUS_API CVarRegistry
	{
	public:
		using Value = std::variant<bool, int, float, std::string>;
		using Getter = std::function<Value()>;
		using Setter = std::function<bool(const Value&)>;

		[[nodiscard]] bool Register(std::string name, std::string description, Getter getter, Setter setter = {});
		[[nodiscard]] bool Get(std::string_view name, std::string& value, std::string& error) const;
		[[nodiscard]] bool Set(std::string_view name, std::string_view value, std::string& error);
		[[nodiscard]] bool Execute(std::string_view command, std::vector<std::string>& output);
		[[nodiscard]] std::vector<std::string> Complete(std::string_view prefix) const;
		static std::string Format(const Value& value);

	private:
		struct Entry
		{
			std::string description;
			Getter getter;
			Setter setter;
			size_t type;
		};
		std::map<std::string, Entry, std::less<>> m_entries;
	};

	// Bind after Config initialization; destroy the registry before Config teardown.
	DAEDALUS_API void RegisterConfigCVars(CVarRegistry& registry);
}
