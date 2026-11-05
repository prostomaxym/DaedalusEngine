#include "dlpch.h"
#include "SceneComposer.h"
#include "SceneJson.h"
#include "Utils/VirtualFileSystem.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <set>

using namespace Daedalus;

namespace
{
	bool ApplyOverrides(SceneRecord& record, const std::vector<EntityOverrideRecord>& overrides, std::string& error)
	{
		for (const auto& patch : overrides)
		{
			const auto found = std::find_if(record.entities.begin(), record.entities.end(), [&](const auto& entity) { return entity.id == patch.id; });
			if (found == record.entities.end())
			{
				error = "Unknown prefab override entity " + patch.id;
				return false;
			}
			auto& entity = *found;
			if (patch.name) entity.name = *patch.name;
			if (patch.transform) entity.transform = *patch.transform;
			if (patch.parent) entity.parent = patch.parent->empty() ? std::nullopt : patch.parent;
			if (patch.links) entity.links = *patch.links;
			if (patch.scripts) entity.scripts = *patch.scripts;
			if (patch.model) entity.model = patch.model;
			if (patch.camera) entity.camera = patch.camera;
			if (patch.directional_light) entity.directional_light = patch.directional_light;
			if (patch.point_light) entity.point_light = patch.point_light;
			if (patch.spot_light) entity.spot_light = patch.spot_light;
			if (patch.skybox) entity.skybox = patch.skybox;
			if (patch.rigid_body) entity.rigid_body = patch.rigid_body;
			if (patch.kinematic_body) entity.kinematic_body = patch.kinematic_body;
			if (patch.character_controller) entity.character_controller = patch.character_controller;
			if (patch.joints) entity.joints = *patch.joints;
			for (const auto& component : patch.remove)
			{
				if (component == "model") entity.model.reset();
				else if (component == "camera") entity.camera.reset();
				else if (component == "directional_light") entity.directional_light.reset();
				else if (component == "point_light") entity.point_light.reset();
				else if (component == "spot_light") entity.spot_light.reset();
				else if (component == "skybox") entity.skybox.reset();
				else if (component == "rigid_body") entity.rigid_body.reset();
				else if (component == "kinematic_body") entity.kinematic_body.reset();
				else if (component == "character_controller") entity.character_controller.reset();
				else if (component == "joints") entity.joints.clear();
			}
		}
		return SceneJson::Validate(record, error);
	}

	struct ActivePaths
	{
		std::set<std::string>& active;
		std::vector<std::string> paths;
		~ActivePaths()
		{
			for (const auto& path : paths) active.erase(path);
		}
	};

	struct Composition
	{
		const VirtualFileSystem& files;
		size_t bytes{ 0 };
		size_t instances{ 0 };
		size_t entities{ 0 };
		std::set<std::string> active;
		bool select_camera{ false };

		bool Fail(std::string& error, const std::string& message) const
		{
			error = message;
			return false;
		}

		bool LoadTemplate(const std::string& reference, SceneRecord& record, size_t& depth, ActivePaths& opened, std::string& error)
		{
			const auto path = VirtualFileSystem::Normalize(reference.find(":/") == std::string::npos ? "assets:/" + reference : reference);
			if (!path) return Fail(error, "Invalid prefab path: " + reference);
			if (depth >= SceneComposer::MaxPrefabDepth) return Fail(error, "Prefab depth limit exceeded at " + *path);
			++depth;
			if (++instances > SceneComposer::MaxPrefabInstances) return Fail(error, "Prefab instance limit exceeded at " + *path);
			if (!active.insert(*path).second) return Fail(error, "Prefab reference cycle at " + *path);
			opened.paths.push_back(*path);
			if (bytes >= SceneComposer::MaxPrefabBytes) return Fail(error, "Prefab byte limit exceeded at " + *path);
			std::string text;
			if (!files.Read(*path, text, error, SceneComposer::MaxPrefabBytes - bytes))
			{
				error = "Prefab " + *path + ": " + error;
				return false;
			}
			bytes += text.size();
			if (!SceneJson::Parse(text, record, error))
			{
				error = "Prefab " + *path + ": " + error;
				return false;
			}
			if (record.entities.size() > SceneComposer::MaxEntities - entities)
				return Fail(error, "Prefab entity limit exceeded at " + *path);
			entities += record.entities.size();
			return ResolveVariant(record, depth, opened, error);
		}

		bool ResolveVariant(SceneRecord& record, size_t& depth, ActivePaths& opened, std::string& error)
		{
			if (!record.base_prefab) return true;
			SceneRecord base;
			if (!LoadTemplate(*record.base_prefab, base, depth, opened, error)) return false;
			if (!ApplyOverrides(base, record.overrides, error)) return false;
			if (!record.name.empty()) base.name = record.name;
			if (record.gravity) base.gravity = record.gravity;
			record = std::move(base);
			return true;
		}

		bool Expand(SceneRecord& record, size_t depth, std::string& error)
		{
			ActivePaths base_paths{ active, {} };
			if (!ResolveVariant(record, depth, base_paths, error)) return false;
			std::set<std::string> ids;
			for (const auto& entity : record.entities) ids.insert(entity.id);
			uint64_t next_id = 1;
			const auto authored_count = record.entities.size();
			for (size_t index = 0; index < authored_count; ++index)
			{
				auto& anchor = record.entities[index];
				if (!anchor.prefab) continue;
				const auto prefab = *anchor.prefab;
				ActivePaths opened{ active, {} };
				auto imported_depth = depth;
				SceneRecord imported;
				if (!LoadTemplate(prefab.path, imported, imported_depth, opened, error)) return false;
				const auto& path = opened.paths.front();
				if (!ApplyOverrides(imported, prefab.overrides, error)) return false;
				if (!Expand(imported, imported_depth, error)) return false;

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
					if (found == remap.end()) return Fail(error, "Unknown prefab export entity " + target + " in " + path);
					if (!anchor.links.emplace(name, found->second).second)
						return Fail(error, "Prefab export would overwrite link " + anchor.id + "." + name);
				}
				if (!imported.primary_camera.empty())
				{
					const auto& camera = remap.at(imported.primary_camera);
					anchor.links.try_emplace("camera", camera);
					if (record.primary_camera == anchor.id && !anchor.camera) record.primary_camera = camera;
					if (select_camera && record.primary_camera.empty()) record.primary_camera = camera;
				}
				else if (record.primary_camera == anchor.id && !anchor.camera)
					return Fail(error, "Selected prefab instance has no primary camera: " + anchor.id);
				for (auto& entity : imported.entities)
				{
					const auto template_id = entity.id;
					for (auto& origin : entity.prefab_origins) origin.instance = remap.at(origin.instance);
					entity.prefab_origins.insert(entity.prefab_origins.begin(), PrefabOrigin{ path, anchor.id, template_id });
					entity.id = remap.at(entity.id);
					entity.parent = entity.parent ? remap.at(*entity.parent) : anchor.id;
					for (auto& [name, target] : entity.links) target = remap.at(target);
					for (auto& joint : entity.joints) joint.target = remap.at(joint.target);
				}
				anchor.prefab_origins.push_back({ path, anchor.id, {} });
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

bool SceneComposer::ExpandPrefab(const PrefabRecord& prefab, const VirtualFileSystem& files, SceneRecord& record, std::string& error)
{
	SceneRecord source;
	EntityRecord anchor;
	anchor.id = "1";
	anchor.name = "Prefab";
	anchor.prefab = prefab;
	source.entities.push_back(std::move(anchor));
	if (!SceneJson::Validate(source, error)) return false;
	Composition composition{ files, 0, 0, 1, {}, true };
	if (!composition.Expand(source, 0, error)) return false;
	record = std::move(source);
	return true;
}
