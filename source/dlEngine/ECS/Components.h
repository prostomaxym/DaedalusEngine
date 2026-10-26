#pragma once

#include "UUID.h"
#include "PhysicsBodyComponent.h"

#include "Renderer/Resources/ResourceManager.h"
#include "Renderer/Objects/Cubemap.h"
#include "Renderer/Objects/DirectionalLightSource.h"
#include "Renderer/Objects/PointLightSource.h"
#include "Renderer/Objects/SpotLightSource.h"
#include "Renderer/Objects/PerspectiveCamera.h"
#include "Renderer/Objects/OrthographicCamera.h"
#include "Renderer/Resourses/Model.h"
#include "Renderer/Resourses/Shader.h"
#include "Physics/Physics.h"

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

		glm::quat GetOrientationQuat() const
		{
			// glm::quat constructor expects radians for Euler angles
			return glm::quat(glm::radians(rotation));
		}

		void SetOrientationQuat(const glm::quat& q)
		{
			// glm::eulerAngles returns radians; convert to degrees and store as (pitch, yaw, roll)
			rotation = glm::degrees(glm::eulerAngles(q));
		}
	};

	struct RenderableObjectComponent
	{
		std::shared_ptr<Model> model;
		std::filesystem::path source_path;
		ModelParserFlags import_flags = ModelParserFlags::NONE;

		RenderableObjectComponent() = default;
		RenderableObjectComponent(const RenderableObjectComponent&) = default;
		explicit RenderableObjectComponent(const std::filesystem::path& path, ModelParserFlags parser_flags = ModelParserFlags::NONE)
			: model(ResourceManager::LoadModel(path, parser_flags)), source_path(path), import_flags(parser_flags) {}
	};

	struct DAEDALUS_API SkyboxComponent //TODO: investigate how to remove exporting
	{
		Cubemap cubemap;
		float rotation_angle{ 0.f };
		std::vector<std::string> source_faces;

		SkyboxComponent() = default;
		SkyboxComponent(const SkyboxComponent&) = default;
		explicit SkyboxComponent(const std::vector<std::string>& faces, float rotation_ang = 0.f)
			: cubemap(faces), rotation_angle(rotation_ang), source_faces(faces) {}
	};

	struct CameraComponent
	{
		std::unique_ptr<Camera> camera;
		bool viewport_aspect = true;

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

	struct RigidBodyComponent : PhysicsBodyComponent
	{
		RigidBodyComponent(float mass, const glm::vec3& halfExtents, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
			: PhysicsBodyComponent(Box(mass, halfExtents, position, orientation), false) {}

		RigidBodyComponent(float mass, float radius, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
			: PhysicsBodyComponent(Sphere(mass, radius, position, orientation), false) {}

		explicit RigidBodyComponent(const BodyRecord& record) : PhysicsBodyComponent(record, false) {}

		RigidBodyComponent() = default;
		RigidBodyComponent(const RigidBodyComponent&) = delete;
		RigidBodyComponent(RigidBodyComponent&&) noexcept = default;
		RigidBodyComponent& operator=(const RigidBodyComponent&) = delete;
		RigidBodyComponent& operator=(RigidBodyComponent&&) noexcept = default;

	};

	struct KinematicBodyComponent : PhysicsBodyComponent
	{
		KinematicBodyComponent(const glm::vec3& halfExtents, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
			: PhysicsBodyComponent(Box(0.f, halfExtents, position, orientation), true) {}

		KinematicBodyComponent(float radius, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
			: PhysicsBodyComponent(Sphere(0.f, radius, position, orientation), true) {}

		explicit KinematicBodyComponent(const BodyRecord& record) : PhysicsBodyComponent(record, true) {}

		KinematicBodyComponent() : PhysicsBodyComponent(BodyRecord{}, true) {}
		KinematicBodyComponent(const KinematicBodyComponent&) = delete;
		KinematicBodyComponent(KinematicBodyComponent&&) noexcept = default;
		KinematicBodyComponent& operator=(const KinematicBodyComponent&) = delete;
		KinematicBodyComponent& operator=(KinematicBodyComponent&&) noexcept = default;

	};
}
