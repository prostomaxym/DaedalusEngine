#pragma once

#include "Macros.h"

#include <filesystem>
#include <string>
#include "Core/AssetRegistry.h"

namespace Daedalus
{
	class Scene;
	struct SceneRecord;
	class ScriptRegistry;
	class AssetPipeline;

	class DAEDALUS_API SceneSerializer
	{
	public:
		// Resource instantiation runs on the application thread after renderer/physics initialization.
		static bool Instantiate(const SceneRecord& record, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error, const AssetPipeline* pipeline = nullptr);
		static bool LoadAsset(const AssetPipeline& pipeline, std::string_view path, Scene& scene, const ScriptRegistry& scripts, std::string& error);
		static bool LoadAsset(const AssetPipeline& pipeline, AssetId id, Scene& scene, const ScriptRegistry& scripts, std::string& error);
		static bool Capture(Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, SceneRecord& record, std::string& error);
		static bool Load(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
		static bool Save(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
	};
}
