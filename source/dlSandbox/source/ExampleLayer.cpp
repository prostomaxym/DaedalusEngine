#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

#include "ExampleScripts.h"

using namespace Daedalus;

namespace
{
	ModelParserFlags enhance_model_flags =
		ModelParserFlags::JOIN_IDENTICAL_VERTICES |
		ModelParserFlags::TRIANGULATE |
		ModelParserFlags::GEN_SMOOTH_NORMALS |
		ModelParserFlags::SPLIT_LARGE_MESHES |
		ModelParserFlags::OPTIMIZE_MESHES |
		ModelParserFlags::OPTIMIZE_GRAPH;

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

	auto light_entity = m_scene.CreateEntity("Main Light");
	light_entity.AddComponent<LightComponent>(Renderer::s_shader_library->Get(ShaderConstants::StandardShader), glm::vec3(-60000.f, 100000.f, 80000.f),
		glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 3.f);

	auto nuke_entity = m_scene.CreateEntity("Nuke");
	nuke_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Nuke/Nuke.obj", enhance_model_flags);
	auto& nuke_transfrom = nuke_entity.GetComponent<TransformComponent>();
	nuke_transfrom.scale = glm::vec3(2.f, 2.f, 2.f);
	nuke_transfrom.translation = glm::vec3(-50.f, 18.f, -120.f);
	nuke_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);

	auto kratos_model = m_scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj", enhance_model_flags);
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(0.f, -2.8f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_entity = m_scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", enhance_model_flags);
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-10.f, -2.8f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RotationModelScript>(miranda_entity, 0.1f);

	auto tank_entity = m_scene.CreateEntity("Tank");
	tank_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj", enhance_model_flags);
	auto& tank_transform = tank_entity.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(3.f, 3.f, 3.f);
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