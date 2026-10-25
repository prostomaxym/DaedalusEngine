#pragma once

#include "SceneRecord.h"
#include "ScriptRegistry.h"

#include <filesystem>

namespace Daedalus
{
	class Scene;

	class DAEDALUS_API SceneSerializer
	{
	public:
		// Resource instantiation runs on the application thread after renderer/physics initialization.
		static bool Instantiate(const SceneRecord& record, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
		static bool Capture(Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, SceneRecord& record, std::string& error);
		static bool Load(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
		static bool Save(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
	};
}
