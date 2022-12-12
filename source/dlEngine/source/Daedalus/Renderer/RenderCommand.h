#pragma once

#include "Daedalus/Renderer/RendererAPI.h"

#include <memory>

namespace Daedalus {

	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

	private:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};

}
