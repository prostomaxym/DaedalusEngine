#pragma once

#include "UUID.h"
#include "Daedalus/Renderer/Objects/LightSource.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
#include "Daedalus/Renderer/Resourses/Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <string>
#include <typeindex>
#include <type_traits>

namespace Daedalus
{
	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; //pitch, yaw, roll degrees angle
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: translation(translation) {}

		glm::mat4 GetTransform() const
		{

			glm::mat4 rotation_mat = glm::toMat4(glm::quat(glm::radians(rotation)));

			return glm::translate(glm::mat4(1.0f), translation)
				* rotation_mat
				* glm::scale(glm::mat4(1.0f), scale);
		}
	};

	struct RenderableObjectComponent
	{
		Model model;

		RenderableObjectComponent() = default;	
		RenderableObjectComponent(const RenderableObjectComponent&) = default;
		RenderableObjectComponent(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE) : model(path, parser_flags) {}
	};

	struct CameraComponent
	{
		PerspectiveCamera camera;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
		CameraComponent(CameraProjectionProps proj_props = CameraProjectionProps(), CameraPositionProps pos_props = CameraPositionProps()) :
			camera(proj_props, pos_props) {}
	};

	struct LightComponent
	{
		LightSource light;

		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
		LightComponent(std::shared_ptr<Shader> shader) : light(shader) {};
		LightComponent(std::shared_ptr<Shader> shader, const glm::vec3& light_pos, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color, float light_power) :
			light(shader, light_pos, ambient_color, diffuse_color, specular_color, light_power) {};
	};


	class NativeScript; //To avoid circular dependency

	struct NativeScriptComponent
	{
		template <typename T, typename... Args>
		T* AddScript(Entity ent, Args&&... args)
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			DL_ASSERT(!ScriptExists<T>(), Log::Categories::ECS, "NativeScriptComponent already has this script!");

			native_scripts[typeid(T)] = std::make_shared<T>(ent, std::forward<Args>(args)...);		 
			return dynamic_cast<T*>(native_scripts[typeid(T)].get());
		}

		template <typename T>
		void RemoveScript()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			DL_ASSERT(ScriptExists<T>(), Log::Categories::ECS, "NativeScriptComponent does not have this script!");

			native_scripts.erase(typeid(T));
		}

		template <typename T>
		T* GetScript()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");

			const auto it = native_scripts.find(typeid(T));
			DL_ASSERT(it != native_scripts.end(), Log::Categories::ECS, "NativeScriptComponent does not have this script!");
			return it != native_scripts.end() ? dynamic_cast<T*>(it->second.get()) : nullptr;
		}

		template <typename T>
		bool ScriptExists()
		{
			static_assert(std::is_base_of_v<NativeScript, T>, "T must be derived from NativeScript");
			return native_scripts.find(typeid(T)) != native_scripts.end();
		}

		// TODO: It is not compiling with unique_ptr, something inside entt tries to copy it.
		// Review and replace it with unique_ptr or convert to static polymorhism
		std::unordered_map<std::type_index, std::shared_ptr<NativeScript>> native_scripts;
	};
}