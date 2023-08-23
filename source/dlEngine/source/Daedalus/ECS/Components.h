#pragma once

#include "UUID.h"
#include "Daedalus/Renderer/Objects/Cubemap.h"
#include "Daedalus/Renderer/Objects/DirectionalLightSource.h"
#include "Daedalus/Renderer/Objects/PointLightSource.h"
#include "Daedalus/Renderer/Objects/SpotLightSource.h"
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
		std::shared_ptr<Shader> shader;

		RenderableObjectComponent() = default;
		RenderableObjectComponent(const RenderableObjectComponent&) = default;
		RenderableObjectComponent(const std::filesystem::path& path, std::shared_ptr<Shader> shader_, ModelParserFlags parser_flags = ModelParserFlags::NONE)
			: model(path, parser_flags), shader(shader_){}
	};

	struct CubemapComponent
	{
		Cubemap cubemap;
		std::shared_ptr<Shader> shader;
		float rotation_angle{ 0.f };

		CubemapComponent() = default;
		CubemapComponent(const CubemapComponent&) = default;
		CubemapComponent(std::vector<std::string>& faces, std::shared_ptr<Shader> shader_, float rotation_ang = 0.f)
			: cubemap(faces), shader(shader_), rotation_angle(rotation_ang){}
	};

	struct CameraComponent
	{
		PerspectiveCamera camera;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
		CameraComponent(CameraProjectionProps proj_props = CameraProjectionProps(), CameraPositionProps pos_props = CameraPositionProps()) :
			camera(proj_props, pos_props) {}
	};

	struct DirectionalLightComponent
	{
		DirectionalLightSource light;
		bool is_dynamic{ false };

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
		DirectionalLightComponent(const glm::vec3& light_dir, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color, float light_power, bool is_dyn) :
			light(light_dir, ambient_color, diffuse_color, specular_color, light_power), is_dynamic(is_dyn) {};
	};

	struct PointLightComponent
	{
		PointLightSource light;
		bool is_dynamic{ false };

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent&) = default;
		PointLightComponent(const glm::vec3& light_pos, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color, float light_power, float max_distance, bool is_dyn) :
			light(light_pos, ambient_color, diffuse_color, specular_color, light_power, max_distance), is_dynamic(is_dyn) {};
	};

	struct SpotLightComponent
	{
		SpotLightSource light;
		bool is_dynamic{ false };

		SpotLightComponent() = default;
		SpotLightComponent(const SpotLightComponent&) = default;
		SpotLightComponent(const glm::vec3& light_pos, const glm::vec3& ambient_color, const glm::vec3& diffuse_color, const glm::vec3& specular_color,
			float light_power, float max_distance, const glm::vec3& direction, float theta_angle, float outer_cutoff, bool is_dyn) :
			light(light_pos, ambient_color, diffuse_color, specular_color, light_power, max_distance, direction, theta_angle, outer_cutoff), is_dynamic(is_dyn) {};
	};
}
