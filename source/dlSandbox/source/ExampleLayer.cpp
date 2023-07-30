#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"

using namespace Daedalus;

namespace
{
	//Create complex level from multiple related obj files
	void CreateEntitiesForOBJFiles(const std::string& folderPath, Scene& scene, const TransformComponent& transform)
	{
		namespace fs = std::filesystem;

		for (const auto& entry : fs::directory_iterator(folderPath))
		{
			if (entry.path().extension() == ".obj")
			{
				std::string filename = entry.path().stem().string();

				auto model_entity = scene.CreateEntity(filename);
				model_entity.AddComponent<RenderableObjectComponent>(entry.path().string());

				auto& model_transform = model_entity.GetComponent<TransformComponent>();
				model_transform = transform;
			}
		}
	}
	//CreateEntitiesForOBJFiles("C:/maxym/objects/Anor Londo/", m_scene, model_transform);


	// Just some examples of basic scripts

	class RorationModelScript : public NativeScript
	{
	public:
		RorationModelScript(Entity entity, float speed) : NativeScript(entity), m_speed(speed) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			m_entity.GetComponent<TransformComponent>().rotation.y += m_speed * dt.GetMilliseconds();
		}

		float m_speed{ 0.f };
	};

	class LogPositionScript : public NativeScript
	{
	public:
		LogPositionScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto& camera = m_entity.GetComponent<CameraComponent>().camera;
			const auto pos = camera.GetPosition();
			const auto message = std::string("Position - ") + "X: " + std::to_string(pos.x) + " / " + "Y: " + std::to_string(pos.y) + " / " + "Z: " + std::to_string(pos.z);
			Log::Write(Log::Levels::Trace, Log::Categories::Application, message);
		}
	};

	class TankMovementScript : public NativeScript
	{
	public:
		TankMovementScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto& transform = m_entity.GetComponent<TransformComponent>();

			auto delta_movement = dt.GetSeconds() * 5.f;
			auto delta_rotation = dt.GetSeconds() * 180.f * 0.3f;

			if (m_current_length_pos <= 50.f && !is_rotating && moving_upwards)
			{
				delta_movement = std::min(delta_movement, 50.f - m_current_length_pos);
				m_current_length_pos += delta_movement;
				transform.translation.x += delta_movement;
				if (m_current_length_pos >= 50.f)
				{
					is_rotating = true;
				}
			}
			else if (m_current_length_pos >=0 && !is_rotating && !moving_upwards)
			{
				delta_movement = std::min(delta_movement, m_current_length_pos);
				m_current_length_pos -= delta_movement;
				transform.translation.x -= delta_movement;
				if (m_current_length_pos <= 0.f)
				{
					is_rotating = true;
				}
			}

			if (m_current_length_pos >= 50.f && is_rotating)
			{
				delta_rotation = std::min(delta_rotation, 180.f - m_current_angle);
				m_current_angle += delta_rotation;
				transform.rotation.y -= delta_rotation;
				if (m_current_angle >= 180.f)
				{
					is_rotating = false;
					moving_upwards = false;
				}
			}
			else if (m_current_length_pos <= 0.f && is_rotating)
			{
				delta_rotation = std::min(delta_rotation, m_current_angle);
				m_current_angle -= delta_rotation;
				transform.rotation.y += delta_rotation;
				if (m_current_angle <= 0.f)
				{
					is_rotating = false;
					moving_upwards = true;
				}
			}
		}

		bool is_rotating{ false };
		bool moving_upwards{ true };
		float m_current_length_pos{ 0.f };
		float m_current_angle{ 0.f };
	};
}

void ExampleLayer::OnAttach()
{
	auto camera_entity = m_scene.CreateEntity("Main Camera");
	camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, Application::GetInstance()->GetWindow().GetAspectRatio(), 0.1f, 1000.f));
	auto& camera = camera_entity.GetComponent<CameraComponent>().camera;
	camera.SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera.RotateCamera(-90.f, 0.f);
	camera.SetMovementSpeed(15.f);
	camera.SetSensivitity(5.f);
	camera.SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	camera_scripts.AddScript<LogPositionScript>(camera_entity);

	TransformComponent model_transform;
	model_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	model_transform.translation = glm::vec3(1300.f, -300.f, 500.f);
	model_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto nuke_entity = m_scene.CreateEntity("Nuke");
	nuke_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Nuke/Nuke.obj", ModelParserFlags::TRIANGULATE);
	auto& nuke_transfrom = nuke_entity.GetComponent<TransformComponent>();
	nuke_transfrom.scale = glm::vec3(2.f, 2.f, 2.f);
	nuke_transfrom.translation = glm::vec3(-50.f, 18.f, -120.f);
	nuke_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);

	auto kratos_model = m_scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj");
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(0.f, -2.8f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_entity = m_scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj");
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-10.f, -2.8f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RorationModelScript>(miranda_entity, 0.1f);

	auto tank_entity = m_scene.CreateEntity("Tank");
	tank_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj");
	auto& tank_transform = tank_entity.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	tank_transform.translation = glm::vec3(-20.f, -2.8f, -25.f);
	tank_transform.rotation = glm::vec3(0.f, 90.f, 0.f);
	auto& tank_scripts = tank_entity.AddComponent<NativeScriptComponent>();
	tank_scripts.AddScript<TankMovementScript>(tank_entity);

	m_scene.OnRuntimeStart();
}

void ExampleLayer::OnDetach()
{
	m_scene.OnRuntimeStop();
}

void ExampleLayer::OnUpdate()
{
	static Timer timer;
	const auto dt = timer.GetEllapsedTime();
	timer.StartTimer();

	m_scene.OnUpdateRuntime(dt);
}