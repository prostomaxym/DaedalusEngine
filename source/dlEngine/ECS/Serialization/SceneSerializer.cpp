#include "dlpch.h"
#include "SceneSerializer.h"
#include "SceneRecord.h"
#include "ScriptRegistry.h"
#include "Renderer/Resources/Texture.h"
#include "SceneJson.h"
#include "ECS/Scene.h"
#include "Utils/Utf8.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>

using namespace Daedalus;

namespace
{
	Daedalus::UUID Id(const std::string& text)
	{
		uint64_t value = 0;
		std::from_chars(text.data(), text.data() + text.size(), value);
		return Daedalus::UUID(value);
	}

	bool Within(const std::filesystem::path& path, const std::filesystem::path& root, std::filesystem::path& relative)
	{
		relative = path.lexically_relative(root);
		if (relative.empty() || relative.is_absolute()) return false;
		for (const auto& part : relative)
			if (part == "..") return false;
		return true;
	}

	bool Resolve(const std::filesystem::path& assets, const std::string& reference, std::filesystem::path& path, std::string& error)
	{
		std::error_code ec;
		const auto root = std::filesystem::weakly_canonical(assets, ec);
		if (!ec) path = std::filesystem::weakly_canonical(root / Utf8ToPath(reference), ec);
		std::filesystem::path relative;
		if (ec || !Within(path, root, relative) || !std::filesystem::is_regular_file(path, ec) || ec)
		{
			error = "Missing asset or path outside asset root: " + reference;
			return false;
		}
		return true;
	}

	bool Reference(const std::filesystem::path& assets, const std::filesystem::path& path, std::string& reference, std::string& error)
	{
		std::error_code ec;
		const auto root = std::filesystem::weakly_canonical(assets, ec);
		const auto canonical = ec ? std::filesystem::path() : std::filesystem::weakly_canonical(path, ec);
		std::filesystem::path relative;
		if (ec || !Within(canonical, root, relative))
		{
			error = "Cannot save an asset outside the asset root: " + Utf8ToString(path.u8string());
			return false;
		}
		reference = Utf8ToString(relative.generic_u8string());
		return true;
	}

	LightProps Props(const LightRecord& r)
	{
		LightProps p;
		p.light_pos = r.position; p.ambient_color = r.ambient; p.diffuse_color = r.diffuse;
		p.specular_color = r.specular; p.direction = r.direction; p.light_power = r.power;
		p.max_distance = r.max_distance; p.theta_angle = r.inner_angle; p.outer_cutoff = r.outer_angle;
		p.cast_shadows = r.cast_shadows; p.number_of_shadow_cascades = static_cast<int>(r.cascades);
		return p;
	}

	LightRecord Light(const LightSource& light)
	{
		const auto& p = light.GetProps();
		LightRecord r;
		r.position = p.light_pos; r.ambient = p.ambient_color; r.diffuse = p.diffuse_color;
		r.specular = p.specular_color; r.direction = p.direction; r.power = p.light_power;
		r.max_distance = p.max_distance; r.inner_angle = p.theta_angle; r.outer_angle = p.outer_cutoff;
		r.cast_shadows = p.cast_shadows; r.cascades = static_cast<uint32_t>(p.number_of_shadow_cascades);
		return r;
	}

	void AddCamera(Entity entity, const CameraRecord& r)
	{
		CameraPositionProps position(r.position, r.up, r.yaw, r.pitch);
		auto& component = r.projection == "perspective"
			? entity.AddComponent<CameraComponent>(CameraProjectionProps(r.fov, r.aspect_ratio, r.near_plane, r.far_plane), position)
			: entity.AddComponent<CameraComponent>(r.left, r.right, r.bottom, r.top, r.near_plane, r.far_plane, position);
		component.viewport_aspect = r.viewport_aspect;
		component.camera->SetMovementSpeed(r.movement_speed);
		component.camera->SetSensivitity(r.sensitivity);
		component.camera->SetZoomSpeed(r.zoom_speed);
		component.camera->SetPersonParams(r.distance, r.height);
	}

	bool GetCamera(const CameraComponent& component, CameraRecord& r, std::string& error)
	{
		const auto* camera = component.camera.get();
		if (!camera) { error = "Cannot save a null camera"; return false; }
		r.position = camera->GetPosition(); r.up = camera->GetWorldUp(); r.yaw = camera->GetYaw(); r.pitch = camera->GetPitch();
		r.near_plane = camera->GetNearPlane(); r.far_plane = camera->GetFarPlane();
		r.movement_speed = camera->GetMovementSpeed(); r.sensitivity = camera->GetSensitivity(); r.zoom_speed = camera->GetZoomSpeed();
		r.distance = camera->GetPersonDistance(); r.height = camera->GetPersonHeight(); r.viewport_aspect = component.viewport_aspect;
		if (const auto* perspective = dynamic_cast<const PerspectiveCamera*>(camera))
		{
			r.fov = perspective->GetProjectionProps().fov;
			r.aspect_ratio = perspective->GetProjectionProps().aspect_ratio;
		}
		else if (const auto* orthographic = dynamic_cast<const OrthographicCamera*>(camera))
		{
			r.projection = "orthographic";
			r.left = orthographic->GetLeftPlane(); r.right = orthographic->GetRightPlane();
			r.bottom = orthographic->GetBottomPlane(); r.top = orthographic->GetTopPlane();
		}
		else { error = "Cannot save an unsupported camera class"; return false; }
		return true;
	}

	template<typename T>
	bool AddBody(Entity entity, const BodyRecord& source, std::string& error)
	{
		auto resolved = source;
		if (source.shape == "model_box")
		{
			resolved.shape = "box";
			resolved.half_extents = entity.GetComponent<RenderableObjectComponent>().model->GetBoundingAABB().extents * source.bounds_scale + source.bounds_offset;
		}
		else if (source.shape == "model_sphere")
		{
			resolved.shape = "sphere";
			resolved.radius = entity.GetComponent<RenderableObjectComponent>().model->GetBoundingSphere().radius;
		}
		if ((resolved.shape == "box" && (!std::isfinite(resolved.half_extents.x) || !std::isfinite(resolved.half_extents.y) ||
			!std::isfinite(resolved.half_extents.z) || resolved.half_extents.x <= 0.f || resolved.half_extents.y <= 0.f || resolved.half_extents.z <= 0.f)) ||
			!std::isfinite(resolved.radius) || resolved.radius <= 0.f)
		{
			error = "Invalid resolved collider dimensions for " + entity.GetName();
			return false;
		}
		auto& component = entity.AddComponent<T>(resolved);
		component.description = source;
		if (!component.handle) { error = "Cannot create physics body for " + entity.GetName(); return false; }
		return true;
	}

	const std::set<entt::id_type>& SupportedComponents()
	{
		static const std::set<entt::id_type> types{
			entt::type_hash<IDComponent>::value(), entt::type_hash<TagComponent>::value(), entt::type_hash<TransformComponent>::value(),
			entt::type_hash<RenderableObjectComponent>::value(), entt::type_hash<CameraComponent>::value(), entt::type_hash<SkyboxComponent>::value(),
			entt::type_hash<DirectionalLightComponent>::value(), entt::type_hash<PointLightComponent>::value(), entt::type_hash<SpotLightComponent>::value(),
			entt::type_hash<RigidBodyComponent>::value(), entt::type_hash<KinematicBodyComponent>::value(), entt::type_hash<NativeScriptComponent>::value()
		};
		return types;
	}
}

bool SceneSerializer::Instantiate(const SceneRecord& record, Scene& scene, const std::filesystem::path& assets,
	const ScriptRegistry& scripts, std::string& error)
{
	error.clear();
	if (scene.IsRunning() || !scene.IsEmpty()) { error = "Scene loading requires an empty, stopped scene"; return false; }
	if (!SceneJson::Validate(record, error)) return false;
	for (const auto& entity : record.entities)
	{
		if (!scripts.Validate(entity, error)) return false;
		std::filesystem::path path;
		if (entity.model && !Resolve(assets, entity.model->path, path, error)) return false;
		if (entity.skybox)
			for (const auto& face : entity.skybox->faces)
				if (!Resolve(assets, face, path, error)) return false;
	}
	const auto previous_name = scene.GetSceneName();
	const auto previous_paused = scene.IsPaused();
	const auto rollback = [&]()
	{
		scene.Clear();
		scene.SetSceneName(previous_name);
		scene.SetPaused(previous_paused);
		return false;
	};
	for (const auto& r : record.entities)
	{
		auto entity = scene.CreateEntityWithUUID(Id(r.id), r.name);
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.translation = r.transform.translation; transform.rotation = r.transform.rotation; transform.scale = r.transform.scale;
		if (r.model)
		{
			std::filesystem::path path;
			if (!Resolve(assets, r.model->path, path, error)) return rollback();
			if (!entity.AddComponent<RenderableObjectComponent>(path, static_cast<ModelParserFlags>(r.model->import_flags)).model)
			{
				error = "Failed to load model for entity " + r.id + ": " + r.model->path;
				return rollback();
			}
		}
		if (r.camera) AddCamera(entity, *r.camera);
		if (r.directional_light) entity.AddComponent<DirectionalLightComponent>(Props(*r.directional_light));
		if (r.point_light) entity.AddComponent<PointLightComponent>(Props(*r.point_light));
		if (r.spot_light) entity.AddComponent<SpotLightComponent>(Props(*r.spot_light));
		if (r.skybox)
		{
			std::vector<std::string> faces;
			for (const auto& face : r.skybox->faces)
			{
				std::filesystem::path path;
				if (!Resolve(assets, face, path, error)) return rollback();
				faces.push_back(Utf8ToString(path.u8string()));
			}
			const auto& cubemap = entity.AddComponent<SkyboxComponent>(faces, r.skybox->rotation).cubemap.GetCubemapTexture();
			if (!cubemap || !cubemap->IsLoaded()) { error = "Failed to load skybox for entity " + r.id; return rollback(); }
		}
		if (r.rigid_body && !AddBody<RigidBodyComponent>(entity, *r.rigid_body, error)) return rollback();
		if (r.kinematic_body && !AddBody<KinematicBodyComponent>(entity, *r.kinematic_body, error)) return rollback();
	}
	scene.SetSceneName(record.name);
	if (!record.primary_camera.empty() && !scene.SetPrimaryCamera(Id(record.primary_camera)))
	{
		error = "Failed to select the primary camera";
		return rollback();
	}
	// Every entity and ordinary component exists before any client script is constructed.
	for (const auto& r : record.entities)
	{
		if (r.scripts.empty()) continue;
		auto entity = scene.GetEntityByUUID(Id(r.id));
		entity.AddComponent<NativeScriptComponent>();
		for (const auto& script : r.scripts)
			if (!scripts.Instantiate(entity, script, error)) return rollback();
	}
	return true;
}

bool SceneSerializer::Capture(Scene& scene, const std::filesystem::path& assets,
	const ScriptRegistry& scripts, SceneRecord& record, std::string& error)
{
	error.clear();
	if (scene.IsRunning()) { error = "Stop the scene before saving an authored definition"; return false; }
	for (const auto& storage : scene.m_registry.storage())
		if (!SupportedComponents().count(storage.first) && storage.second.size() != 0)
		{
			error = "Cannot save an unsupported component storage";
			return false;
		}
	SceneRecord result;
	result.name = scene.m_scene_name;
	for (const auto handle : scene.m_registry.view<IDComponent>())
	{
		Entity entity(handle, &scene);
		EntityRecord r;
		r.id = std::to_string(static_cast<uint64_t>(entity.GetUUID())); r.name = entity.GetName();
		const auto& transform = entity.GetComponent<TransformComponent>();
		r.transform = { transform.translation, transform.rotation, transform.scale };
		if (entity.HasComponent<RenderableObjectComponent>())
		{
			const auto& component = entity.GetComponent<RenderableObjectComponent>();
			if (!component.model || component.source_path.empty()) { error = "Model has no persistent asset reference"; return false; }
			r.model.emplace();
			if (!Reference(assets, component.source_path, r.model->path, error)) return false;
			r.model->import_flags = static_cast<uint32_t>(component.import_flags);
		}
		if (entity.HasComponent<CameraComponent>())
		{
			r.camera.emplace();
			if (!GetCamera(entity.GetComponent<CameraComponent>(), *r.camera, error)) return false;
			if (entity.GetUUID() == scene.m_primary_camera || (static_cast<uint64_t>(scene.m_primary_camera) == 0 && r.name == "Main Camera"))
				result.primary_camera = r.id;
		}
		if (entity.HasComponent<DirectionalLightComponent>()) r.directional_light = Light(entity.GetComponent<DirectionalLightComponent>().light);
		if (entity.HasComponent<PointLightComponent>()) r.point_light = Light(entity.GetComponent<PointLightComponent>().light);
		if (entity.HasComponent<SpotLightComponent>()) r.spot_light = Light(entity.GetComponent<SpotLightComponent>().light);
		if (entity.HasComponent<SkyboxComponent>())
		{
			const auto& component = entity.GetComponent<SkyboxComponent>();
			r.skybox.emplace(); r.skybox->rotation = component.rotation_angle;
			for (const auto& face : component.source_faces)
			{
				std::string reference;
				if (!Reference(assets, Utf8ToPath(face), reference, error)) return false;
				r.skybox->faces.push_back(std::move(reference));
			}
		}
		if (entity.HasComponent<RigidBodyComponent>()) r.rigid_body = entity.GetComponent<RigidBodyComponent>().description;
		if (entity.HasComponent<KinematicBodyComponent>()) r.kinematic_body = entity.GetComponent<KinematicBodyComponent>().description;
		if (entity.HasComponent<NativeScriptComponent>() && !scripts.Capture(entity.GetComponent<NativeScriptComponent>(), r.scripts, error)) return false;
		if (!scripts.Validate(r, error)) return false;
		result.entities.push_back(std::move(r));
	}
	std::sort(result.entities.begin(), result.entities.end(), [](const auto& a, const auto& b) { return static_cast<uint64_t>(Id(a.id)) < static_cast<uint64_t>(Id(b.id)); });
	if (!SceneJson::Validate(result, error)) return false;
	record = std::move(result);
	return true;
}

bool SceneSerializer::Load(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
	const ScriptRegistry& scripts, std::string& error)
{
	SceneRecord record;
	return SceneJson::Load(path, record, error) && Instantiate(record, scene, assets, scripts, error);
}

bool SceneSerializer::Save(const std::filesystem::path& path, Scene& scene, const std::filesystem::path& assets,
	const ScriptRegistry& scripts, std::string& error)
{
	SceneRecord record;
	return Capture(scene, assets, scripts, record, error) && SceneJson::Save(path, record, error);
}
