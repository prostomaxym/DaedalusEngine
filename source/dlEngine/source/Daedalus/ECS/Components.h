#pragma once

#include "UUID.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
#include "Daedalus/Renderer/Resourses/Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

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
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation_mat = glm::toMat4(glm::quat(rotation));

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


	class ScriptableEntity; //To avoid circular dependency

	struct NativeScriptComponent
	{
		ScriptableEntity* instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->instance; nsc->instance = nullptr; };
		}
	};
}