#include "SceneScripts.h"
#include "ExampleScripts.h"

#include <cmath>
#include <limits>

using namespace Daedalus;

namespace
{
	bool CameraScript(const ScriptRecord& script, const EntityRecord& entity, std::string& error)
	{
		if (!script.properties.empty() || !entity.camera)
		{
			error = "requires a camera and no properties";
			return false;
		}
		return true;
	}

	bool Number(const ScriptRecord& script, const char* name)
	{
		const auto it = script.properties.find(name);
		if (it == script.properties.end()) return false;
		const auto* value = std::get_if<double>(&it->second);
		return value && std::isfinite(*value) && std::abs(*value) <= std::numeric_limits<float>::max();
	}

	template<typename T>
	T* Create(Entity entity, const ScriptRecord&)
	{
		return entity.GetComponent<NativeScriptComponent>().AddScript<T>(entity);
	}

	template<typename T>
	void CaptureEmpty(const T&, ScriptRecord&) {}
}

bool Daedalus::RegisterSceneScripts(ScriptRegistry& registry)
{
	return registry.Register<CameraController>("CameraController", CameraScript, Create<CameraController>, CaptureEmpty<CameraController>) &&
		registry.Register<LogPositionScript>("LogPositionScript", CameraScript, Create<LogPositionScript>, CaptureEmpty<LogPositionScript>) &&
		registry.Register<PlayerControllerScript>("PlayerControllerScript",
			[](const ScriptRecord& script, const EntityRecord& entity, std::string& error)
			{
				if (!CameraScript(script, entity, error)) return false;
				if (!entity.model || !entity.kinematic_body || !entity.spot_light)
				{
					error = "requires model, kinematic_body and spot_light components";
					return false;
				}
				return true;
			}, Create<PlayerControllerScript>, CaptureEmpty<PlayerControllerScript>) &&
		registry.Register<RotationModelScript>("RotationModelScript",
			[](const ScriptRecord& script, const EntityRecord&, std::string& error)
			{
				if (script.properties.size() != 1 || !Number(script, "speed"))
				{
					error = "requires exactly one numeric property: speed";
					return false;
				}
				return true;
			},
			[](Entity entity, const ScriptRecord& script)
			{
				return entity.GetComponent<NativeScriptComponent>().AddScript<RotationModelScript>(entity,
					static_cast<float>(std::get<double>(script.properties.at("speed"))));
			},
			[](const RotationModelScript& script, ScriptRecord& record) { record.properties["speed"] = static_cast<double>(script.GetSpeed()); }) &&
		registry.Register<TankMovementScript>("TankMovementScript",
			[](const ScriptRecord& script, const EntityRecord&, std::string& error)
			{
				const auto it = script.properties.find("forward_direction");
				const auto* direction = it == script.properties.end() ? nullptr : std::get_if<glm::vec3>(&it->second);
				if (script.properties.size() != 2 || !Number(script, "distance") || std::get<double>(script.properties.at("distance")) <= 0.0 ||
					!direction || glm::dot(*direction, *direction) < 0.000001f)
				{
					error = "requires positive distance and nonzero forward_direction";
					return false;
				}
				return true;
			},
			[](Entity entity, const ScriptRecord& script)
			{
				return entity.GetComponent<NativeScriptComponent>().AddScript<TankMovementScript>(entity,
					std::get<glm::vec3>(script.properties.at("forward_direction")), static_cast<float>(std::get<double>(script.properties.at("distance"))));
			},
			[](const TankMovementScript& script, ScriptRecord& record)
			{
				record.properties["forward_direction"] = script.GetForwardDirection();
				record.properties["distance"] = static_cast<double>(script.GetMaxDistance());
			});
}
