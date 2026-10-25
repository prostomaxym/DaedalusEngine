#pragma once

#include "Macros.h"
#include "SceneRecord.h"

#include <filesystem>
#include <string_view>

namespace Daedalus
{
	class DAEDALUS_API SceneJson
	{
	public:
		static bool Parse(std::string_view text, SceneRecord& record, std::string& error);
		static bool Serialize(const SceneRecord& record, std::string& text, std::string& error);
		static bool Validate(const SceneRecord& record, std::string& error);
		static bool Load(const std::filesystem::path& path, SceneRecord& record, std::string& error);
		static bool Save(const std::filesystem::path& path, const SceneRecord& record, std::string& error);
	};
}
