#include "ExampleLayer.h"

#include "Daedalus/Core/Input.h"
#include "Daedalus/Renderer/API/Renderer.h"

void ExampleLayer::OnAttach()
{
	m_camera = Daedalus::PerspectiveCamera(Daedalus::CameraProjectionProps(80.f * 3.14f / 180.f, 16.f / 9.f, 0.1f, 1000.f), Daedalus::CameraPositionProps());
	m_camera.SetPosition(glm::vec3(-1.f, 0.f, 0.f));

	m_model = Daedalus::Model("../../../assets/models/Kratos/Kratos.obj");
}

void ExampleLayer::OnUpdate()
{
	const auto model_transform = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));

	auto test_shader = Daedalus::Renderer::s_shader_library->Get("TestShader");

	if (Daedalus::Input::IsKeyPressed(DL_KEY_W))
		m_camera.MoveCamera(Daedalus::CameraMovement::FORWARD, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_S))
		m_camera.MoveCamera(Daedalus::CameraMovement::BACKWARD, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_A))
		m_camera.MoveCamera(Daedalus::CameraMovement::LEFT, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_D))
		m_camera.MoveCamera(Daedalus::CameraMovement::RIGHT, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_SPACE))
		m_camera.MoveCamera(Daedalus::CameraMovement::UP, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_C))
		m_camera.MoveCamera(Daedalus::CameraMovement::DOWN, 0.02f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_Q))
		m_camera.RotateCamera(-0.3f, 0.f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_E))
		m_camera.RotateCamera(0.3f, 0.f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_R))
		m_camera.RotateCamera(0.0f, 0.2f);

	if (Daedalus::Input::IsKeyPressed(DL_KEY_F))
		m_camera.RotateCamera(0.0f, -0.2f);

	Daedalus::RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
	Daedalus::RenderCommand::Clear();

	Daedalus::Renderer::BeginScene(m_camera);
	Daedalus::Renderer::Submit(test_shader.get(), &m_model);
	Daedalus::Renderer::EndScene();

	test_shader->Unbind();
}