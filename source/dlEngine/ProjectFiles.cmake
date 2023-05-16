set(PROJECT_FILES

    source/Daedalus.h
    source/dlpch.h
    source/dlpch.cpp
    source/DummyTest.h

    source/Daedalus/Core/Application.h
    source/Daedalus/Core/Application.cpp
    source/Daedalus/Core/Core.h
    source/Daedalus/Core/EntryPoint.h
    source/Daedalus/Core/Input.h
    source/Daedalus/Core/Layer.h
    source/Daedalus/Core/Layer.cpp
    source/Daedalus/Core/LayerStack.h
    source/Daedalus/Core/LayerStack.cpp
    source/Daedalus/Core/Window.h  
    
    source/Daedalus/Debug/Log.h
    source/Daedalus/Debug/Log.cpp

    source/Daedalus/Events/ApplicationEvent.h
    source/Daedalus/Events/Event.h
    source/Daedalus/Events/Event.cpp
    source/Daedalus/Events/EventDispatcher.h
    source/Daedalus/Events/EventTable.h
    source/Daedalus/Events/KeyEvent.h
    source/Daedalus/Events/MouseEvent.h
    source/Daedalus/Events/GamepadEvent.h
    source/Daedalus/Events/EventsEngine.h
    source/Daedalus/Events/EventsEngine.cpp

    source/Daedalus/Renderer/API/RenderCommand.h
    source/Daedalus/Renderer/API/RenderCommand.cpp
    source/Daedalus/Renderer/API/Renderer.h
    source/Daedalus/Renderer/API/Renderer.cpp
    source/Daedalus/Renderer/API/RendererAPI.h
    source/Daedalus/Renderer/API/RendererAPI.cpp

    source/Daedalus/Renderer/Objects/Camera.h
    source/Daedalus/Renderer/Objects/Camera.cpp
    source/Daedalus/Renderer/Objects/OrthographicCamera.h
    source/Daedalus/Renderer/Objects/OrthographicCamera.cpp
    source/Daedalus/Renderer/Objects/PerspectiveCamera.h
    source/Daedalus/Renderer/Objects/PerspectiveCamera.cpp

    source/Daedalus/Renderer/Primitives/BufferLayout.h
    source/Daedalus/Renderer/Primitives/GraphicsContext.h
    source/Daedalus/Renderer/Primitives/GraphicsContext.cpp
    source/Daedalus/Renderer/Primitives/FrameBuffer.h
    source/Daedalus/Renderer/Primitives/FrameBuffer.cpp
    source/Daedalus/Renderer/Primitives/IndexBuffer.h
    source/Daedalus/Renderer/Primitives/IndexBuffer.cpp
    source/Daedalus/Renderer/Primitives/UniformBuffer.h
    source/Daedalus/Renderer/Primitives/UniformBuffer.cpp
    source/Daedalus/Renderer/Primitives/VertexArray.h
    source/Daedalus/Renderer/Primitives/VertexArray.cpp
    source/Daedalus/Renderer/Primitives/VertexBuffer.h
    source/Daedalus/Renderer/Primitives/VertexBuffer.cpp

    source/Daedalus/Renderer/Resourses/Mesh.h
    source/Daedalus/Renderer/Resourses/Mesh.cpp
    source/Daedalus/Renderer/Resourses/Model.h
    source/Daedalus/Renderer/Resourses/Model.cpp
    source/Daedalus/Renderer/Resourses/Shader.h
    source/Daedalus/Renderer/Resourses/Shader.cpp
    source/Daedalus/Renderer/Resourses/ShaderLibrary.h
    source/Daedalus/Renderer/Resourses/ShaderLibrary.cpp
    source/Daedalus/Renderer/Resourses/Texture.h
    source/Daedalus/Renderer/Resourses/Texture.cpp
    
    source/Daedalus/UI/ImGui/ImGuiLayer.h
    source/Daedalus/UI/ImGui/ImGuiLayer.cpp

    source/Daedalus/Utils/HighPerformanceRequest.h
    source/Daedalus/Utils/KeyCodes.h
    source/Daedalus/Utils/WorkingDirectory.h
    source/Daedalus/Utils/WorkingDirectory.cpp
    
    source/Platform/Platform.h
    source/Platform/Platform.cpp

    source/Platform/GLFW/GLFWInput.h
    source/Platform/GLFW/GLFWInput.cpp
    source/Platform/GLFW/GLFWWindow.h
    source/Platform/GLFW/GLFWWindow.cpp

    source/Platform/OpenGL/OpenGLContext.h
    source/Platform/OpenGL/OpenGLContext.cpp
    source/Platform/OpenGL/OpenGLFrameBuffer.h
    source/Platform/OpenGL/OpenGLFrameBuffer.cpp
    source/Platform/OpenGL/OpenGLIndexBuffer.h
    source/Platform/OpenGL/OpenGLIndexBuffer.cpp
    source/Platform/OpenGL/OpenGLRendererAPI.h
    source/Platform/OpenGL/OpenGLRendererAPI.cpp
    source/Platform/OpenGL/OpenGLShader.h
    source/Platform/OpenGL/OpenGLShader.cpp
    source/Platform/OpenGL/OpenGLShaderLibrary.h
    source/Platform/OpenGL/OpenGLShaderLibrary.cpp
    source/Platform/OpenGL/OpenGLTexture.h
    source/Platform/OpenGL/OpenGLTexture.cpp
    source/Platform/OpenGL/OpenGLUniformBuffer.h
    source/Platform/OpenGL/OpenGLUniformBuffer.cpp
    source/Platform/OpenGL/OpenGLVertexArray.h
    source/Platform/OpenGL/OpenGLVertexArray.cpp
    source/Platform/OpenGL/OpenGLVertexBuffer.h
    source/Platform/OpenGL/OpenGLVertexBuffer.cpp

    source/Platform/Windows/WindowsInput.h
    source/Platform/Windows/WindowsWindow.h
    source/Platform/Windows/WindowsWindow.cpp

    source/Platform/Linux/LinuxInput.h
    source/Platform/Linux/LinuxWindow.h
    source/Platform/Linux/LinuxWindow.cpp
    )