#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Daedalus
{
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

	struct BodyRecord
	{
		// Model-derived shapes use unscaled bounds, matching the existing sandbox.
		std::string shape = "box";
		float mass = 0.f;
		glm::vec3 half_extents{ 0.5f };
		float radius = 0.5f;
		glm::vec3 bounds_scale{ 1.f };
		glm::vec3 bounds_offset{ 0.f };
		glm::vec3 position{ 0.f };
		glm::vec3 rotation{ 0.f };
	};

	using ScriptProperty = std::variant<bool, double, std::string, glm::vec3>;

	struct ScriptRecord
	{
		std::string type;
		std::map<std::string, ScriptProperty> properties;
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
		std::vector<ScriptRecord> scripts;
	};

	struct SceneRecord
	{
		uint32_t version = 1;
		std::string name;
		std::string primary_camera;
		std::vector<EntityRecord> entities;
	};
}
