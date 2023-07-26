#include "ExampleLayer.h"

using namespace Daedalus;

void ExampleLayer::OnAttach()
{
	auto camera = m_scene.CreateEntity("Main Camera");
	camera.AddComponent<CameraComponent>(CameraProjectionProps(80.f * 3.14f / 180.f, 16.f / 9.f, 0.1f, 1000.f));
	camera.GetComponent<CameraComponent>().camera.SetPosition(glm::vec3(-1.f, 0.f, 0.f));

	auto test_model = m_scene.CreateEntity("Kratos");
	test_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj");
	auto transform_component = test_model.GetComponent<TransformComponent>();

	transform_component.scale = glm::vec3(1.f, 1.f, 1.f);
	transform_component.translation = glm::vec3(0.f, 1.f, 0.f);
	transform_component.rotation = glm::vec3(0.f, 0.f, 0.f);

	m_scene.OnRuntimeStart();
}

void ExampleLayer::OnDetach()
{
	m_scene.OnRuntimeStop();
}

void ExampleLayer::OnUpdate()
{
	auto& camera = m_scene.FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera;

	if (Input::IsKeyPressed(DL_KEY_W))
		camera.MoveCamera(CameraMovement::FORWARD, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_S))
		camera.MoveCamera(CameraMovement::BACKWARD, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_A))
		camera.MoveCamera(CameraMovement::LEFT, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_D))
		camera.MoveCamera(CameraMovement::RIGHT, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_SPACE))
		camera.MoveCamera(CameraMovement::UP, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_C))
		camera.MoveCamera(CameraMovement::DOWN, 0.02f);

	if (Input::IsKeyPressed(DL_KEY_Q))
		camera.RotateCamera(-0.3f, 0.f);

	if (Input::IsKeyPressed(DL_KEY_E))
		camera.RotateCamera(0.3f, 0.f);

	if (Input::IsKeyPressed(DL_KEY_R))
		camera.RotateCamera(0.0f, 0.2f);

	if (Input::IsKeyPressed(DL_KEY_F))
		camera.RotateCamera(0.0f, -0.2f);

	m_scene.OnUpdateRuntime(DeltaTime::Lock60FPS());
}