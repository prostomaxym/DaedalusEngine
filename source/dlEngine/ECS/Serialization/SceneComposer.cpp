#include "dlpch.h"
#include "SceneComposer.h"
#include "SceneJson.h"
#include "Utils/VirtualFileSystem.h"

#include <iterator>
#include <limits>
#include <map>
#include <set>

using namespace Daedalus;

namespace
{
	struct Composition
	{
		const VirtualFileSystem& files;
		size_t bytes{ 0 };
		size_t instances{ 0 };
		size_t entities{ 0 };
		std::set<std::string> active;

		bool Fail(std::string& error, const std::string& message) const
		{
			error = message;
			return false;
		}

		bool Expand(SceneRecord& record, size_t depth, std::string& error)
		{
			std::set<std::string> ids;
			for (const auto& entity : record.entities) ids.insert(entity.id);
			uint64_t next_id = 1;
			const auto authored_count = record.entities.size();
			for (size_t index = 0; index < authored_count; ++index)
			{
				auto& anchor = record.entities[index];
				if (!anchor.prefab) continue;
				const auto prefab = *anchor.prefab;
				const auto path = VirtualFileSystem::Normalize(prefab.path.find(":/") == std::string::npos ? "assets:/" + prefab.path : prefab.path);
				if (!path) return Fail(error, "Invalid prefab path: " + prefab.path);
				if (depth >= SceneComposer::MaxPrefabDepth) return Fail(error, "Prefab depth limit exceeded at " + *path);
				if (++instances > SceneComposer::MaxPrefabInstances) return Fail(error, "Prefab instance limit exceeded at " + *path);
				if (!active.insert(*path).second) return Fail(error, "Prefab reference cycle at " + *path);
				if (bytes >= SceneComposer::MaxPrefabBytes) return Fail(error, "Prefab byte limit exceeded at " + *path);
				std::string text;
				if (!files.Read(*path, text, error, SceneComposer::MaxPrefabBytes - bytes))
				{
					error = "Prefab " + *path + ": " + error;
					return false;
				}
				bytes += text.size();
				SceneRecord imported;
				if (!SceneJson::Parse(text, imported, error))
				{
					error = "Prefab " + *path + ": " + error;
					return false;
				}
				if (imported.entities.size() > SceneComposer::MaxEntities - entities)
					return Fail(error, "Prefab entity limit exceeded at " + *path);
				entities += imported.entities.size();
				if (!Expand(imported, depth + 1, error)) return false;
				active.erase(*path);

				std::map<std::string, std::string> remap;
				for (const auto& entity : imported.entities)
				{
					while (ids.contains(std::to_string(next_id)))
					{
						if (next_id == (std::numeric_limits<uint64_t>::max)()) return Fail(error, "No UUID available for prefab expansion");
						++next_id;
					}
					const auto id = std::to_string(next_id);
					ids.insert(id);
					remap.emplace(entity.id, id);
				}
				for (const auto& [name, target] : prefab.exports)
				{
					const auto found = remap.find(target);
					if (found == remap.end()) return Fail(error, "Unknown prefab export entity " + target + " in " + *path);
					if (!anchor.links.emplace(name, found->second).second)
						return Fail(error, "Prefab export would overwrite link " + anchor.id + "." + name);
				}
				if (!imported.primary_camera.empty())
				{
					const auto& camera = remap.at(imported.primary_camera);
					anchor.links.try_emplace("camera", camera);
					if (record.primary_camera == anchor.id && !anchor.camera) record.primary_camera = camera;
				}
				else if (record.primary_camera == anchor.id && !anchor.camera)
					return Fail(error, "Selected prefab instance has no primary camera: " + anchor.id);
				for (auto& entity : imported.entities)
				{
					entity.id = remap.at(entity.id);
					entity.parent = entity.parent ? remap.at(*entity.parent) : anchor.id;
					for (auto& [name, target] : entity.links) target = remap.at(target);
				}
				anchor.prefab.reset();
				record.entities.insert(record.entities.end(), std::make_move_iterator(imported.entities.begin()), std::make_move_iterator(imported.entities.end()));
			}
			return SceneJson::Validate(record, error);
		}
	};
}

bool SceneComposer::Expand(const SceneRecord& source, const VirtualFileSystem& files, SceneRecord& record, std::string& error)
{
	error.clear();
	if (source.entities.size() > MaxEntities)
	{
		error = "Prefab composition entity limit exceeded";
		return false;
	}
	SceneRecord result;
	if (!SceneJson::Upgrade(source, result, error)) return false;
	if (result.entities.size() > MaxEntities)
	{
		error = "Prefab composition entity limit exceeded after migration";
		return false;
	}
	Composition composition{ files, 0, 0, result.entities.size(), {} };
	if (!composition.Expand(result, 0, error)) return false;
	record = std::move(result);
	return true;
}
