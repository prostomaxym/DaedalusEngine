#include "dlpch.h"
#include "Application.h"

#include "Core.h"
#include "Daedalus/Events/EventDispatcher.h"
#include "Platform/Platform.h"

#include <glad/glad.h>
#include "Daedalus/Renderer/VertexBuffer.h"
#include "Daedalus/Renderer/VertexArray.h"
#include "Daedalus/Renderer/IndexBuffer.h"
#include "Daedalus/Renderer/Renderer.h"
#include "Daedalus/Renderer/RendererAPI.h"
#include "Daedalus/Renderer/Shader.h"

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

	m_imgui_layer = new ImGuiLayer();
	PushOverlay(m_imgui_layer);
}

Application::~Application()
{
}

void Application::Run()
{
	//auto test_shader = Shader::Create("C:/Users/ershi/source/repos/prostomaxym/Daedalus/shaders/TestShader.vert", "C:/Users/ershi/source/repos/prostomaxym/Daedalus/shaders/TestShader.frag");
	//test_shader->SaveBinary("C:/Users/ershi/source/repos/prostomaxym/Daedalus/shaders/Cache/TestShader.dlshader");

	auto test_shader = Shader::Create("C:/Users/ershi/source/repos/prostomaxym/Daedalus/shaders/Cache/TestShader.dlshader");
	test_shader->Bind();

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.0f,  0.5f, 0.0f
	};

	auto VBO = VertexBuffer::Create(vertices, sizeof(vertices));
	auto VAO = VertexArray::Create();
	VBO->Bind();
	VAO->Bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	while (m_running)
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		for (auto layer : m_layer_stack)
		{
			layer->OnUpdate();
		}

		m_window->OnUpdate();

		m_imgui_layer->Begin();
		m_imgui_layer->End();
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