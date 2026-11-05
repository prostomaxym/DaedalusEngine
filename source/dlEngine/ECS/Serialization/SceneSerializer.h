#pragma once

#include "Macros.h"
#include "SceneRecord.h"
#include "ECS/UUID.h"

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
		// Prefabs expand before construction; capture saves their expanded entities.
		static bool Instantiate(const SceneRecord& record, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error, const AssetPipeline* pipeline = nullptr);
		// Owner-thread, outside scene iteration. Failure leaves instance unchanged.
		// Factories may inspect the assembled scene, but must not mutate unrelated entities.
		static bool SpawnPrefab(const PrefabRecord& prefab, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, UUID& instance, std::string& error, const TransformRecord& transform = {},
			UUID parent = UUID(0), const AssetPipeline* pipeline = nullptr);
		static bool LoadAsset(const AssetPipeline& pipeline, std::string_view path, Scene& scene, const ScriptRegistry& scripts, std::string& error);
		static bool LoadAsset(const AssetPipeline& pipeline, AssetId id, Scene& scene, const ScriptRegistry& scripts, std::string& error);
		static bool Capture(Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, SceneRecord& record, std::string& error);
		static bool Load(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
		static bool Save(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error);
	private:
		static bool Construct(const SceneRecord& record, Scene& scene, const std::filesystem::path& assets,
			const ScriptRegistry& scripts, std::string& error, const AssetPipeline* pipeline, bool append);
	};
}
