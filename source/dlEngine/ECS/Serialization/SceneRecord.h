#pragma once

#include <glm/glm.hpp>
#include "Physics/PhysicsTypes.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Daedalus
{
	inline constexpr uint32_t CurrentSceneVersion = 5;

	struct TransformRecord
	{
		glm::vec3 translation{ 0.f };
		glm::vec3 rotation{ 0.f };
		glm::vec3 scale{ 1.f };
	};

	struct ModelRecord
	{
		std::string path;
		uint32_t import_flags = 0;
	};

	struct CameraRecord
	{
		std::string projection = "perspective";
		glm::vec3 position{ 0.f };
		glm::vec3 up{ 0.f, 1.f, 0.f };
		float yaw = 0.f;
		float pitch = 0.f;
		float fov = 70.f;
		float aspect_ratio = 16.f / 9.f;
		bool viewport_aspect = true;
		float near_plane = 0.1f;
		float far_plane = 200.f;
		float left = -1.f;
		float right = 1.f;
		float bottom = -1.f;
		float top = 1.f;
		float movement_speed = 3.f;
		float sensitivity = 2.f;
		float zoom_speed = 1.f;
		float distance = 0.f;
		float height = 0.f;
	};

	struct LightRecord
	{
		glm::vec3 position{ 0.f };
		glm::vec3 ambient{ 0.f };
		glm::vec3 diffuse{ 1.f };
		glm::vec3 specular{ 1.f };
		glm::vec3 direction{ 0.f, -1.f, 0.f };
		float power = 1.f;
		float max_distance = 100.f;
		float inner_angle = 10.f;
		float outer_angle = 35.f;
		bool cast_shadows = false;
		uint32_t cascades = 1;
	};

	struct SkyboxRecord
	{
		std::vector<std::string> faces;
		float rotation = 0.f;
	};

	struct BodyRecord : ColliderDescription
	{
		// Model-derived shapes use unscaled bounds, matching the existing game examples.
		float mass = 0.f;
		PhysicsBodySettings settings;
		glm::vec3 bounds_scale{ 1.f };
		glm::vec3 bounds_offset{ 0.f };
		glm::vec3 position{ 0.f };
		glm::vec3 rotation{ 0.f };
	};

	struct JointRecord : JointSettings
	{
		std::string target;
	};

	using ScriptProperty = std::variant<bool, double, std::string, glm::vec3>;

	struct ScriptRecord
	{
		std::string type;
		std::map<std::string, ScriptProperty> properties;
	};

	struct PrefabRecord
	{
		std::string path;
		std::map<std::string, std::string> exports;
		std::vector<struct EntityOverrideRecord> overrides;
	};

	struct EntityOverrideRecord
	{
		std::string id;
		std::optional<std::string> name;
		std::optional<TransformRecord> transform;
		// An empty parent string explicitly makes the template entity a root.
		std::optional<std::string> parent;
		std::optional<std::map<std::string, std::string>> links;
		std::optional<std::vector<ScriptRecord>> scripts;
		std::optional<ModelRecord> model;
		std::optional<CameraRecord> camera;
		std::optional<LightRecord> directional_light;
		std::optional<LightRecord> point_light;
		std::optional<LightRecord> spot_light;
		std::optional<SkyboxRecord> skybox;
		std::optional<BodyRecord> rigid_body;
		std::optional<BodyRecord> kinematic_body;
		std::optional<CharacterSettings> character_controller;
		std::optional<std::vector<JointRecord>> joints;
		std::vector<std::string> remove;
	};

	struct PrefabOrigin
	{
		std::string path;
		std::string instance;
		std::string template_entity;
	};

	struct EntityRecord
	{
		std::string id;
		std::string name = "Entity";
		TransformRecord transform;
		std::optional<ModelRecord> model;
		std::optional<CameraRecord> camera;
		std::optional<LightRecord> directional_light;
		std::optional<LightRecord> point_light;
		std::optional<LightRecord> spot_light;
		std::optional<SkyboxRecord> skybox;
		std::optional<BodyRecord> rigid_body;
		std::optional<BodyRecord> kinematic_body;
		std::optional<CharacterSettings> character_controller;
		std::vector<JointRecord> joints;
		std::vector<ScriptRecord> scripts;
		std::optional<std::string> parent;
		std::map<std::string, std::string> links;
		std::optional<PrefabRecord> prefab;
		// Composition metadata only; JSON and expanded capture omit this lineage.
		std::vector<PrefabOrigin> prefab_origins;
	};

	struct SceneRecord
	{
		uint32_t version = CurrentSceneVersion;
		std::string name;
		std::string primary_camera;
		std::optional<glm::vec3> gravity;
		std::vector<EntityRecord> entities;
		std::optional<std::string> base_prefab;
		std::vector<EntityOverrideRecord> overrides;
	};
}
