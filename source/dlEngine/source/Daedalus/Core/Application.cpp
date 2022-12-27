#include "dlpch.h"
#include "Application.h"

#include "Core.h"
#include "Daedalus/Events/EventDispatcher.h"

#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Resourses/Shader.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RendererAPI.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
#include "Daedalus/Utils/KeyCodes.h"

#include "Platform/Platform.h"

using namespace Daedalus;

Application* Application::s_instance = nullptr;

Application* Application::GetInstance()
{
	if (!s_instance)
	{
		s_instance = new Application();
	}

	return s_instance;
}

Application::Application() :
	m_window(Platform::createWindow())
{
	s_instance = this;

	Platform::InitInputSystem();
	m_window->SetEventCallback(DL_BIND_EVENT_FN(Application::OnEvent));

	Renderer::Init();
	Renderer::LoadShaderLibrary("../../../shaders/");

	m_imgui_layer = new ImGuiLayer();
	PushOverlay(m_imgui_layer);
}

Application::~Application()
{
	Renderer::Shutdown();
}

void Application::Run()
{
	auto test_shader = Renderer::s_shader_library->Get("TestShader");
	test_shader->Bind();

	PerspectiveCamera pcamera(CameraProjectionProps(45.f * 3.14f / 180.f, 16.f/9.f, 0.1f, 1000.f), CameraPositionProps());
	pcamera.SetPosition(glm::vec3(-1.f, 0.f, 0.f));

	const auto model = std::make_shared<Model>("C:/maxym/objects/Kratos/Kratos.obj");
	const auto model_transform = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
	
	while (m_running)
	{
		for (auto layer : m_layer_stack)
		{
			layer->OnUpdate();
		}

		if (Input::IsKeyPressed(DL_KEY_W))
			pcamera.MoveCamera(CameraMovement::FORWARD, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_S))
			pcamera.MoveCamera(CameraMovement::BACKWARD, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_A))
			pcamera.MoveCamera(CameraMovement::LEFT, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_D))
			pcamera.MoveCamera(CameraMovement::RIGHT, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_SPACE))
			pcamera.MoveCamera(CameraMovement::UP, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_C))
			pcamera.MoveCamera(CameraMovement::DOWN, 0.02f);

		if (Input::IsKeyPressed(DL_KEY_Q))
			pcamera.RotateCamera(-0.3f, 0.f);

		if (Input::IsKeyPressed(DL_KEY_E))
			pcamera.RotateCamera(0.3f, 0.f);

		if (Input::IsKeyPressed(DL_KEY_R))
			pcamera.RotateCamera(0.0f, 0.2f);

		if (Input::IsKeyPressed(DL_KEY_F))
			pcamera.RotateCamera(0.0f, -0.2f);

		RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
		RenderCommand::Clear();

		Renderer::BeginScene(pcamera);
		Renderer::Submit(test_shader.get(), model.get());

		Renderer::EndScene();

		m_imgui_layer->Begin();
		m_imgui_layer->End();

		m_window->OnUpdate();
	}
}

void Application::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowCloseEvent>(DL_BIND_EVENT_FN(Application::OnWindowClosed));

	DL_CORE_TRACE("{0}", event);
	for (auto it = m_layer_stack.end(); it != m_layer_stack.begin();)
	{
		if (event.IsHandled())
			break;

		(*--it)->OnEvent(event);
	}
}

void Application::PushLayer(Layer* layer)
{
	m_layer_stack.PushLayer(layer);
	layer->OnAttach();
}

void Application::PushOverlay(Layer* overlay)
{
	m_layer_stack.PushOverlay(overlay);
	overlay->OnAttach();
}

bool Application::OnWindowClosed(WindowCloseEvent& event)
{
	m_running = false;
	return true;
}