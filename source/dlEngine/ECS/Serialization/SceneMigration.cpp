#include "dlpch.h"
#include "SceneJson.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <iterator>
#include <limits>
#include <set>

using namespace Daedalus;

namespace
{
	glm::vec3 Rotation(const glm::vec3& direction, glm::vec3 up)
	{
		const auto front = glm::normalize(glm::dvec3(direction));
		if (glm::length(glm::cross(front, glm::dvec3(up))) < 0.000001) up = { 0.f, 0.f, 1.f };
		const auto view = glm::lookAt(glm::dvec3(0.0), front, glm::dvec3(up));
		return glm::vec3(glm::degrees(glm::eulerAngles(glm::normalize(glm::quat_cast(glm::transpose(glm::dmat3(view)))))));
	}
}

bool SceneJson::Upgrade(const SceneRecord& source, SceneRecord& record, std::string& error)
{
	if (!Validate(source, error)) return false;
	SceneRecord result = source;
	if (source.version == 2)
	{
		record = std::move(result);
		return true;
	}

	std::set<std::string> ids;
	std::set<std::string> parents;
	for (const auto& entity : source.entities)
	{
		ids.insert(entity.id);
		if (entity.parent) parents.insert(*entity.parent);
	}
	uint64_t next_id = 1;
	std::vector<EntityRecord> added;
	for (auto& entity : result.entities)
	{
		const auto pose_count = static_cast<unsigned>(entity.camera.has_value()) + static_cast<unsigned>(entity.directional_light.has_value())
			+ static_cast<unsigned>(entity.point_light.has_value()) + static_cast<unsigned>(entity.spot_light.has_value());
		const bool split = pose_count > 1 || entity.model || entity.rigid_body || entity.kinematic_body || entity.skybox || parents.contains(entity.id) || entity.parent;
		auto destination = [&](const char* name) -> EntityRecord*
		{
			if (!split) return &entity;
			if (entity.links.contains(name))
			{
				error = "Legacy migration would overwrite entity link: " + entity.id + "." + name;
				return nullptr;
			}
			while (ids.contains(std::to_string(next_id)))
			{
				if (next_id == (std::numeric_limits<uint64_t>::max)())
				{
					error = "No entity ID available for scene migration";
					return nullptr;
				}
				++next_id;
			}
			EntityRecord target;
			target.id = std::to_string(next_id);
			target.name = entity.name + " / " + name;
			ids.insert(target.id);
			entity.links.emplace(name, target.id);
			added.push_back(std::move(target));
			return &added.back();
		};
		if (entity.camera)
		{
			const auto camera = *entity.camera;
			auto* target = destination("camera");
			if (!target) return false;
			const glm::vec3 front{ std::cos(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)),
				std::sin(glm::radians(camera.pitch)), std::sin(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)) };
			target->transform.translation = camera.position;
			target->transform.rotation = Rotation(front, camera.up);
			target->camera = camera;
			if (result.primary_camera == entity.id) result.primary_camera = target->id;
			if (split) entity.camera.reset();
		}
		auto migrate_light = [&](std::optional<LightRecord>& light, const char* name, auto member)
		{
			if (!light) return true;
			const auto props = *light;
			auto* target = destination(name);
			if (!target) return false;
			target->transform.translation = props.position;
			if (glm::length(props.direction) > 0.000001f)
				target->transform.rotation = Rotation(props.direction, { 0.f, 1.f, 0.f });
			target->*member = props;
			if (split) light.reset();
			return true;
		};
		if (!migrate_light(entity.directional_light, "directional_light", &EntityRecord::directional_light) ||
			!migrate_light(entity.point_light, "point_light", &EntityRecord::point_light) ||
			!migrate_light(entity.spot_light, "spot_light", &EntityRecord::spot_light)) return false;
	}
	result.entities.insert(result.entities.end(), std::make_move_iterator(added.begin()), std::make_move_iterator(added.end()));
	result.version = 2;
	if (!Validate(result, error)) return false;
	record = std::move(result);
	return true;
}
