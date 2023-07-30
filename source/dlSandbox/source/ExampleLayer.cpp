#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"

using namespace Daedalus;

namespace
{
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

	class RorationModelScript : public NativeScript
	{
	public:
		RorationModelScript(Entity entity, float speed) : NativeScript(entity), m_speed(speed) {}

	protected:

		virtual void OnUpdate(DeltaTime ts) override
		{
			m_entity.GetComponent<TransformComponent>().rotation.y += m_speed * ts.GetMilliseconds();
		}

		float m_speed{ 0.f };
	};

	class LogPositionScript : public NativeScript
	{
	public:
		LogPositionScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime ts) override
		{
			auto& camera = m_entity.GetComponent<CameraComponent>().camera;
			const auto pos = camera.GetPosition();
			const auto message = std::string("Position - ") + "X: " + std::to_string(pos.x) + " / " + "Y: " + std::to_string(pos.y) + " / " + "Z: " + std::to_string(pos.z);
			Log::Write(Log::Levels::Trace, Log::Categories::Application, message);
		}
	};
}

void ExampleLayer::OnAttach()
{
	auto camera = m_scene.CreateEntity("Main Camera");
	camera.AddComponent<CameraComponent>(CameraProjectionProps(80.f * 3.14f / 180.f, 16.f / 9.f, 0.1f, 1000.f));
	auto& pers_camera = camera.GetComponent<CameraComponent>().camera;
	pers_camera.SetPosition(glm::vec3(0.f, 0.f, 0.f));
	pers_camera.RotateCamera(-90.f, 0.f);
	pers_camera.SetMovementSpeed(15.f);
	pers_camera.SetSensivitity(5.f);
	pers_camera.SetZoomSpeed(1.f);
	camera.AddComponent<NativeScriptComponent>().Bind<CameraController>();
	camera.AddComponent<NativeScriptComponent>().Bind<LogPositionScript>();

	TransformComponent model_transform;
	model_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	model_transform.translation = glm::vec3(1300.f, -300.f, 500.f);
	model_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto nuke_model = m_scene.CreateEntity("Nuke");
	nuke_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Nuke/Nuke.obj", ModelParserFlags::TRIANGULATE);
	auto& nuke_transfrom = nuke_model.GetComponent<TransformComponent>();
	nuke_transfrom.scale = glm::vec3(2.f, 2.f, 2.f);
	nuke_transfrom.translation = glm::vec3(-50.f, 18.f, -120.f);
	nuke_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);

	auto kratos_model = m_scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj");
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(0.f, -2.8f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_model = m_scene.CreateEntity("Miranda");
	miranda_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj");
	auto& miranda_transform = miranda_model.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-10.f, -2.8f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	miranda_model.AddComponent<NativeScriptComponent>().Bind<RorationModelScript>(0.1f);

	auto tank_model = m_scene.CreateEntity("Tank");
	tank_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj");
	auto& tank_transform = tank_model.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	tank_transform.translation = glm::vec3(-20.f, -2.8f, -25.f);
	tank_transform.rotation = glm::vec3(0.f, 90.f, 0.f);

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