#pragma once

namespace Daedalus
{
	class Window;

	namespace ImGuiBackend
	{
		void Initialize(Window& window);
		void Shutdown();
		void NewFrame();
		void DrawControls();
		void RenderPlatformWindows();
		const char* ShaderVersion();
	}
}
