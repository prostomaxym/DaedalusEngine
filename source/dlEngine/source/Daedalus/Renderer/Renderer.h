#pragma once

#include "RenderCommand.h"

namespace Daedalus {

class Renderer
{
public:
	static void Init();
	static void Shutdown();


	static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
};
}
