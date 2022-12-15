#include "dlpch.h"
#include "Application.h"

#include "Core.h"
#include "Daedalus/Events/EventDispatcher.h"

#include "Daedalus/Renderer/VertexBuffer.h"
#include "Daedalus/Renderer/VertexArray.h"
#include "Daedalus/Renderer/IndexBuffer.h"
#include "Daedalus/Renderer/Renderer.h"
#include "Daedalus/Renderer/RendererAPI.h"
#include "Daedalus/Renderer/Shader.h"

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

	float buffer[] = {
	-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	 0.5f, -0.5f, 0.0f, 0.0f,  1.0f, 0.0f, 1.0f,
	 0.0f,  0.5f, 0.0f, 0.0f,  0.0f, 1.0f, 1.0f
	};

	unsigned int indexes[] = {
		0,1,2
	};

	auto VAO = VertexArray::Create();
	VAO->Bind();

	auto VBO = VertexBuffer::Create(buffer, sizeof(buffer));
	VBO->SetLayout(BufferLayout{ BufferElement{ ShaderDataType::Float3, std::string("v_vert"), false }, 
		BufferElement{ ShaderDataType::Float4, std::string("v_color"), false } });

	auto EBO = IndexBuffer::Create(indexes, sizeof(indexes) / sizeof(indexes[0]));

	VAO->AddVertexBuffer(VBO);
	VAO->SetIndexBuffer(EBO);
	VAO->Unbind();

	while (m_running)
	{
		for (auto layer : m_layer_stack)
		{
			layer->OnUpdate();
		}

		RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
		RenderCommand::Clear();

		Renderer::BeginScene();
		Renderer::Submit(test_shader, VAO);
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