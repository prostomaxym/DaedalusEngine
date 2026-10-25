#include "dlpch.h"
#include "SceneJson.h"

#include <nlohmann/json.hpp>

#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <type_traits>

using namespace Daedalus;

namespace
{
	using Json = nlohmann::json;

	template<typename T> struct Optional : std::false_type {};
	template<typename T> struct Optional<std::optional<T>> : std::true_type { using Value = T; };
	template<typename T> struct Vector : std::false_type {};
	template<typename T> struct Vector<std::vector<T>> : std::true_type { using Value = T; };

	// A single field list keeps reading, writing and unknown-field checks consistent.
	template<typename T, typename F>
	void Fields(T& r, F&& f)
	{
		using R = std::remove_const_t<T>;
		if constexpr (std::is_same_v<R, TransformRecord>)
		{
			f("translation", r.translation); f("rotation", r.rotation); f("scale", r.scale);
		}
		else if constexpr (std::is_same_v<R, ModelRecord>)
		{
			f("path", r.path); f("import_flags", r.import_flags);
		}
		else if constexpr (std::is_same_v<R, CameraRecord>)
		{
			f("projection", r.projection); f("position", r.position); f("up", r.up);
			f("yaw", r.yaw); f("pitch", r.pitch); f("fov", r.fov);
			f("aspect_ratio", r.aspect_ratio); f("viewport_aspect", r.viewport_aspect);
			f("near_plane", r.near_plane); f("far_plane", r.far_plane);
			f("left", r.left); f("right", r.right); f("bottom", r.bottom); f("top", r.top);
			f("movement_speed", r.movement_speed); f("sensitivity", r.sensitivity);
			f("zoom_speed", r.zoom_speed); f("distance", r.distance); f("height", r.height);
		}
		else if constexpr (std::is_same_v<R, LightRecord>)
		{
			f("position", r.position); f("ambient", r.ambient); f("diffuse", r.diffuse);
			f("specular", r.specular); f("direction", r.direction); f("power", r.power);
			f("max_distance", r.max_distance); f("inner_angle", r.inner_angle);
			f("outer_angle", r.outer_angle); f("cast_shadows", r.cast_shadows); f("cascades", r.cascades);
		}
		else if constexpr (std::is_same_v<R, SkyboxRecord>)
		{
			f("faces", r.faces); f("rotation", r.rotation);
		}
		else if constexpr (std::is_same_v<R, BodyRecord>)
		{
			f("shape", r.shape); f("mass", r.mass); f("half_extents", r.half_extents);
			f("radius", r.radius); f("bounds_scale", r.bounds_scale); f("bounds_offset", r.bounds_offset);
			f("position", r.position); f("rotation", r.rotation);
		}
		else if constexpr (std::is_same_v<R, ScriptRecord>)
		{
			f("type", r.type); f("properties", r.properties);
		}
		else if constexpr (std::is_same_v<R, EntityRecord>)
		{
			f("id", r.id); f("name", r.name); f("transform", r.transform);
			f("model", r.model); f("camera", r.camera);
			f("directional_light", r.directional_light); f("point_light", r.point_light); f("spot_light", r.spot_light);
			f("skybox", r.skybox); f("rigid_body", r.rigid_body); f("kinematic_body", r.kinematic_body); f("scripts", r.scripts);
		}
		else if constexpr (std::is_same_v<R, SceneRecord>)
		{
			f("version", r.version); f("name", r.name); f("primary_camera", r.primary_camera); f("entities", r.entities);
		}
	}

	template<typename T>
	Json Encode(const T& value)
	{
		if constexpr (std::is_same_v<T, glm::vec3>)
			return Json::array({ value.x, value.y, value.z });
		else if constexpr (std::is_same_v<T, ScriptProperty>)
			return std::visit([](const auto& property) { return Encode(property); }, value);
		else if constexpr (Optional<T>::value)
			return value ? Encode(*value) : Json(nullptr);
		else if constexpr (Vector<T>::value)
		{
			auto result = Json::array();
			for (const auto& item : value) result.push_back(Encode(item));
			return result;
		}
		else if constexpr (std::is_same_v<T, std::map<std::string, ScriptProperty>>)
		{
			auto result = Json::object();
			for (const auto& item : value) result[item.first] = Encode(item.second);
			return result;
		}
		else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
			return Json(value);
		else
		{
			auto result = Json::object();
			Fields(value, [&](const char* name, const auto& field)
			{
				using Field = std::decay_t<decltype(field)>;
				if constexpr (Optional<Field>::value)
				{
					if (field) result[name] = Encode(*field);
				}
				else result[name] = Encode(field);
			});
			return result;
		}
	}

	bool Fail(std::string& error, const std::string& path, const std::string& message)
	{
		if (error.empty()) error = path + ": " + message;
		return false;
	}

	bool ValidUtf8(const std::string& text)
	{
		for (size_t i = 0; i < text.size();)
		{
			const auto c = static_cast<unsigned char>(text[i++]);
			if (c < 0x80) continue;
			const unsigned count = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : 0;
			if (!count || i + count > text.size()) return false;
			const auto next = static_cast<unsigned char>(text[i]);
			if ((c == 0xe0 && next < 0xa0) || (c == 0xed && next >= 0xa0) ||
				(c == 0xf0 && next < 0x90) || (c == 0xf4 && next >= 0x90)) return false;
			for (unsigned j = 0; j < count; ++j)
				if ((static_cast<unsigned char>(text[i++]) & 0xc0) != 0x80) return false;
		}
		return true;
	}

	template<typename T>
	bool Decode(const Json& json, T& value, const std::string& path, std::string& error)
	{
		if constexpr (std::is_same_v<T, std::string>)
		{
			if (!json.is_string()) return Fail(error, path, "expected a string");
			value = json.get<std::string>();
			return ValidUtf8(value) || Fail(error, path, "invalid UTF-8");
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			if (!json.is_boolean()) return Fail(error, path, "expected a boolean");
			value = json.get<bool>();
			return true;
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			if (!json.is_number()) return Fail(error, path, "expected a number");
			const double number = json.get<double>();
			if (!std::isfinite(number) || number < std::numeric_limits<T>::lowest() || number > std::numeric_limits<T>::max())
				return Fail(error, path, "number is not finite or is out of range");
			if constexpr (std::is_integral_v<T>)
				if (!json.is_number_integer()) return Fail(error, path, "expected an integer");
			value = static_cast<T>(number);
			return true;
		}
		else if constexpr (std::is_same_v<T, glm::vec3>)
		{
			if (!json.is_array() || json.size() != 3) return Fail(error, path, "expected a three-number array");
			for (size_t i = 0; i < 3; ++i)
				if (!Decode(json[i], value[static_cast<glm::length_t>(i)], path + "[" + std::to_string(i) + "]", error)) return false;
			return true;
		}
		else if constexpr (std::is_same_v<T, ScriptProperty>)
		{
			if (json.is_boolean()) { bool v{}; if (!Decode(json, v, path, error)) return false; value = v; }
			else if (json.is_number()) { double v{}; if (!Decode(json, v, path, error)) return false; value = v; }
			else if (json.is_string()) { std::string v; if (!Decode(json, v, path, error)) return false; value = v; }
			else { glm::vec3 v{}; if (!Decode(json, v, path, error)) return false; value = v; }
			return true;
		}
		else if constexpr (Optional<T>::value)
		{
			typename Optional<T>::Value item;
			if (!Decode(json, item, path, error)) return false;
			value = std::move(item);
			return true;
		}
		else if constexpr (Vector<T>::value)
		{
			if (!json.is_array()) return Fail(error, path, "expected an array");
			for (size_t i = 0; i < json.size(); ++i)
			{
				typename Vector<T>::Value item;
				if (!Decode(json[i], item, path + "[" + std::to_string(i) + "]", error)) return false;
				value.push_back(std::move(item));
			}
			return true;
		}
		else if constexpr (std::is_same_v<T, std::map<std::string, ScriptProperty>>)
		{
			if (!json.is_object()) return Fail(error, path, "expected a property object");
			for (auto it = json.begin(); it != json.end(); ++it)
			{
				ScriptProperty property;
				if (!ValidUtf8(it.key()) || !Decode(it.value(), property, path + "." + it.key(), error))
					return Fail(error, path, "invalid property");
				value.emplace(it.key(), std::move(property));
			}
			return true;
		}
		else
		{
			if (!json.is_object()) return Fail(error, path, "expected an object");
			std::set<std::string> fields;
			bool valid = true;
			Fields(value, [&](const char* name, auto& field)
			{
				fields.insert(name);
				const auto it = json.find(name);
				if (valid && it != json.end()) valid = Decode(*it, field, path + "." + name, error);
			});
			for (auto it = json.begin(); it != json.end(); ++it)
				if (!fields.count(it.key())) return Fail(error, path + "." + it.key(), "unknown field/component");
			return valid;
		}
	}

	bool ValidId(const std::string& id)
	{
		uint64_t number = 0;
		const auto result = std::from_chars(id.data(), id.data() + id.size(), number);
		return !id.empty() && id.front() != '0' && result.ec == std::errc() && result.ptr == id.data() + id.size() && number != 0;
	}

	bool AssetPath(const std::string& value)
	{
		if (value.empty() || value.find('\\') != std::string::npos || value.find(':') != std::string::npos || value.find('\0') != std::string::npos)
			return false;
		const auto path = std::filesystem::u8path(value);
		if (path.is_absolute() || path.has_root_path()) return false;
		for (const auto& part : path)
			if (part == ".." || part == ".") return false;
		return path.generic_u8string() == value;
	}

	bool Positive(const glm::vec3& value)
	{
		return value.x > 0.f && value.y > 0.f && value.z > 0.f;
	}

	// Match the geometry-shader invocation limit in source/shader/ShadowPass.glsl.
	constexpr uint32_t MaxShadowMaps = 32;

	bool CheckLight(const LightRecord& light, const std::string& path, std::string& error, bool spot = false, bool require_direction = true)
	{
		if (spot && light.cascades != 1) return Fail(error, path + ".cascades", "spotlights support exactly one cascade");
		const auto direction_length = glm::length(light.direction);
		if (light.power < 0.f || light.max_distance <= 0.f || light.cascades < 1 || light.cascades > MaxShadowMaps ||
			light.inner_angle < 0.f || light.inner_angle > light.outer_angle || light.outer_angle > 180.f || (spot && light.outer_angle >= 90.f) ||
			(require_direction && (!std::isfinite(direction_length) || direction_length < 0.000001f)) ||
			light.ambient.x < 0.f || light.ambient.y < 0.f || light.ambient.z < 0.f ||
			light.diffuse.x < 0.f || light.diffuse.y < 0.f || light.diffuse.z < 0.f ||
			light.specular.x < 0.f || light.specular.y < 0.f || light.specular.z < 0.f)
			return Fail(error, path, "invalid light colors, direction, range, angles or cascade count (1..32)");
		return true;
	}

	bool CheckBody(const BodyRecord& body, const EntityRecord& entity, bool kinematic, const std::string& path, std::string& error)
	{
		if (body.mass < 0.f || (kinematic && body.mass != 0.f)) return Fail(error, path, "invalid body mass");
		if (body.shape != "box" && body.shape != "sphere" && body.shape != "model_box" && body.shape != "model_sphere")
			return Fail(error, path + ".shape", "unknown collider shape");
		if ((body.shape == "model_box" || body.shape == "model_sphere") && !entity.model)
			return Fail(error, path, "model-derived collider requires a model");
		if ((body.shape == "box" && !Positive(body.half_extents)) || body.radius <= 0.f ||
			body.bounds_scale.x < 0.f || body.bounds_scale.y < 0.f || body.bounds_scale.z < 0.f)
			return Fail(error, path, "invalid collider dimensions");
		return true;
	}
}

bool SceneJson::Validate(const SceneRecord& record, std::string& error)
{
	error.clear();
	SceneRecord checked;
	if (!Decode(Encode(record), checked, "$", error)) return false;
	if (record.version != 1) return Fail(error, "$.version", "unsupported scene version (expected 1)");
	std::set<std::string> ids;
	bool primary_found = record.primary_camera.empty();
	bool has_camera = false;
	uint32_t shadow_maps = 0;
	for (size_t i = 0; i < record.entities.size(); ++i)
	{
		const auto& entity = record.entities[i];
		const auto path = "$.entities[" + std::to_string(i) + "]";
		if (!ValidId(entity.id) || !ids.insert(entity.id).second) return Fail(error, path + ".id", "expected a unique nonzero uint64 decimal string");
		if (entity.name.empty()) return Fail(error, path + ".name", "name must not be empty");
		if (entity.transform.scale.x == 0.f || entity.transform.scale.y == 0.f || entity.transform.scale.z == 0.f)
			return Fail(error, path + ".transform.scale", "scale must be nonzero");
		if (entity.model && !AssetPath(entity.model->path)) return Fail(error, path + ".model.path", "expected a portable asset-root-relative path without traversal");
		if (entity.camera)
		{
			has_camera = true;
			const auto& camera = *entity.camera;
			if (camera.projection != "perspective" && camera.projection != "orthographic") return Fail(error, path + ".camera.projection", "unknown projection");
			if (camera.near_plane >= camera.far_plane || camera.aspect_ratio <= 0.f || camera.fov <= 0.f || camera.fov >= 180.f ||
				(camera.projection == "perspective" && camera.near_plane <= 0.f) || camera.left >= camera.right || camera.bottom >= camera.top ||
				camera.movement_speed < 0.f || camera.sensitivity < 0.f || camera.zoom_speed < 0.f || camera.pitch <= -90.f || camera.pitch >= 90.f)
				return Fail(error, path + ".camera", "invalid projection or camera controls");
			const glm::vec3 front{ std::cos(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)), std::sin(glm::radians(camera.pitch)), std::sin(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)) };
			if (glm::length(glm::cross(front, camera.up)) < 0.000001f) return Fail(error, path + ".camera.up", "up must not be zero or parallel to the view direction");
			if (entity.id == record.primary_camera) primary_found = true;
		}
		if (entity.directional_light && !CheckLight(*entity.directional_light, path + ".directional_light", error)) return false;
		if (entity.point_light && !CheckLight(*entity.point_light, path + ".point_light", error, false, false)) return false;
		if (entity.spot_light && !CheckLight(*entity.spot_light, path + ".spot_light", error, true)) return false;
		if (entity.directional_light && entity.directional_light->cast_shadows) shadow_maps += entity.directional_light->cascades;
		if (entity.point_light && entity.point_light->cast_shadows) shadow_maps += 6;
		if (entity.spot_light && entity.spot_light->cast_shadows) shadow_maps += entity.spot_light->cascades;
		if (shadow_maps > MaxShadowMaps) return Fail(error, path, "scene exceeds the renderer's 32 shadow-map limit");
		if (entity.skybox)
		{
			if (entity.skybox->faces.size() != 6) return Fail(error, path + ".skybox.faces", "expected six faces: right, left, top, bottom, front, back");
			for (const auto& face : entity.skybox->faces)
				if (!AssetPath(face)) return Fail(error, path + ".skybox.faces", "invalid asset path");
		}
		if (entity.rigid_body && entity.kinematic_body) return Fail(error, path, "an entity cannot have both body types");
		if (entity.rigid_body && !CheckBody(*entity.rigid_body, entity, false, path + ".rigid_body", error)) return false;
		if (entity.kinematic_body && !CheckBody(*entity.kinematic_body, entity, true, path + ".kinematic_body", error)) return false;
		std::set<std::string> script_types;
		for (const auto& script : entity.scripts)
			if (script.type.empty() || !script_types.insert(script.type).second) return Fail(error, path + ".scripts", "script names must be nonempty and unique per entity");
	}
	if (!primary_found || (has_camera && record.primary_camera.empty())) return Fail(error, "$.primary_camera", "must reference a camera entity");
	return true;
}

bool SceneJson::Parse(std::string_view text, SceneRecord& record, std::string& error)
{
	error.clear();
	bool duplicate = false;
	std::vector<std::set<std::string>> keys;
	const auto json = Json::parse(text.begin(), text.end(), [&](int, Json::parse_event_t event, Json& parsed)
	{
		if (event == Json::parse_event_t::object_start) keys.emplace_back();
		else if (event == Json::parse_event_t::key && !keys.back().insert(parsed.get<std::string>()).second) duplicate = true;
		else if (event == Json::parse_event_t::object_end) keys.pop_back();
		return true;
	}, false);
	if (json.is_discarded()) return Fail(error, "$", "malformed JSON");
	if (duplicate) return Fail(error, "$", "duplicate JSON object key");
	if (!json.is_object() || !json.contains("version") || !json.contains("entities")) return Fail(error, "$", "version and entities are required");
	SceneRecord result;
	if (!Decode(json, result, "$", error) || !Validate(result, error)) return false;
	record = std::move(result);
	return true;
}

bool SceneJson::Serialize(const SceneRecord& record, std::string& text, std::string& error)
{
	if (!Validate(record, error)) return false;
	text = Encode(record).dump(2) + "\n";
	return true;
}

bool SceneJson::Load(const std::filesystem::path& path, SceneRecord& record, std::string& error)
{
	std::ifstream stream(path, std::ios::binary);
	if (!stream) { error = "Cannot open scene: " + path.u8string(); return false; }
	const std::string text{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	if (stream.bad()) { error = "Cannot read scene: " + path.u8string(); return false; }
	if (!Parse(text, record, error)) { error = path.u8string() + ": " + error; return false; }
	return true;
}

bool SceneJson::Save(const std::filesystem::path& path, const SceneRecord& record, std::string& error)
{
	std::string text;
	if (!Serialize(record, text, error)) return false;
	auto temporary = path; temporary += ".tmp";
	auto backup = path; backup += ".bak";
	std::error_code ec;
	const bool replacing = std::filesystem::exists(path, ec);
	if (ec || (replacing && !std::filesystem::is_regular_file(path, ec)) || ec)
	{
		error = "Scene save target is not a regular file or cannot be inspected: " + path.u8string();
		return false;
	}
	if (std::filesystem::exists(temporary, ec) || ec || std::filesystem::exists(backup, ec) || ec)
	{
		error = "Scene save recovery files already exist or cannot be inspected: " + path.u8string();
		return false;
	}
	{
		std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		stream.close();
		if (!stream)
		{
			std::filesystem::remove(temporary, ec);
			error = "Cannot write scene: " + path.u8string();
			return false;
		}
	}
	if (!ec && replacing) std::filesystem::rename(path, backup, ec);
	if (!ec) std::filesystem::rename(temporary, path, ec);
	if (ec)
	{
		error = "Cannot replace scene: " + path.u8string() + ": " + ec.message();
		std::error_code recovery_error;
		if (replacing && std::filesystem::exists(backup, recovery_error)) std::filesystem::rename(backup, path, recovery_error);
		if (recovery_error) error += "; original preserved at " + backup.u8string();
		std::filesystem::remove(temporary, recovery_error);
		return false;
	}
	if (replacing)
	{
		std::filesystem::remove(backup, ec);
		if (ec) { error = "Scene saved, but backup cleanup failed: " + backup.u8string(); return false; }
	}
	return true;
}
