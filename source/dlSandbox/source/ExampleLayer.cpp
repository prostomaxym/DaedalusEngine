#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"

using namespace Daedalus;

void ExampleLayer::OnAttach()
{
	auto camera = m_scene.CreateEntity("Main Camera");
	camera.AddComponent<CameraComponent>(CameraProjectionProps(80.f * 3.14f / 180.f, 16.f / 9.f, 0.1f, 1000.f));
	auto& pers_camera = camera.GetComponent<CameraComponent>().camera;
	pers_camera.SetPosition(glm::vec3(0.f, 0.f, 0.f));
	pers_camera.RotateCamera(-90.f, 0.f);
	pers_camera.SetMovementSpeed(10.f);
	pers_camera.SetSensivitity(5.f);
	pers_camera.SetZoomSpeed(1.f);
	camera.AddComponent<NativeScriptComponent>();
	camera.GetComponent<NativeScriptComponent>().Bind<CameraController>();

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

	auto& miranda_rot = m_scene.FindEntityByName("Miranda").GetComponent<TransformComponent>().rotation;
	miranda_rot.y += 0.1f * dt.GetMilliseconds();

	m_scene.OnUpdateRuntime(dt);
	FPSLocker::LockFps(144, timer);	
}