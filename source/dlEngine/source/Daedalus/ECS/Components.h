#pragma once

#include "UUID.h"
#include "Daedalus/Renderer/Objects/Cubemap.h"
#include "Daedalus/Renderer/Objects/DirectionalLightSource.h"
#include "Daedalus/Renderer/Objects/PointLightSource.h"
#include "Daedalus/Renderer/Objects/SpotLightSource.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
#include "Daedalus/Renderer/Objects/OrthographicCamera.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Resourses/Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

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
		explicit TagComponent(std::string_view tag)
			: tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; //pitch, yaw, roll degrees angle
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		explicit TransformComponent(const glm::vec3& translation)
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
		explicit RenderableObjectComponent(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE)
			: model(path, parser_flags) {}
	};

	struct SkyboxComponent
	{
		Cubemap cubemap;
		float rotation_angle{ 0.f };

		SkyboxComponent() = default;
		SkyboxComponent(const SkyboxComponent&) = default;
		explicit SkyboxComponent(std::vector<std::string>& faces, float rotation_ang = 0.f)
			: cubemap(faces), rotation_angle(rotation_ang){}
	};

	struct CameraComponent
	{
		std::unique_ptr<Camera> camera;

		CameraComponent() = default;

		explicit CameraComponent(CameraProjectionProps proj_props = CameraProjectionProps(), CameraPositionProps pos_props = CameraPositionProps()) :
			camera(std::make_unique<PerspectiveCamera>(proj_props, pos_props)) {}

		CameraComponent(float left, float right, float bottom, float top, float front, float back, CameraPositionProps pos_props = CameraPositionProps()) :
			camera(std::make_unique<OrthographicCamera>(left, right, bottom, top, front, back, pos_props)) {}
	};

	struct DirectionalLightComponent
	{
		DirectionalLightSource light;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
		explicit DirectionalLightComponent(const LightProps& props) : light(props) {}
	};

	struct PointLightComponent
	{
		PointLightSource light;

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent&) = default;
		explicit PointLightComponent(const LightProps& props) : light(props) {}
	};

	struct SpotLightComponent
	{
		SpotLightSource light;

		SpotLightComponent() = default;
		SpotLightComponent(const SpotLightComponent&) = default;
		explicit SpotLightComponent(const LightProps& props) : light(props) {}
	};
}
