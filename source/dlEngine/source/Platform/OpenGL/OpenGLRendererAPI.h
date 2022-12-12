#pragma once

#include "Daedalus/Renderer/RendererAPI.h"

namespace Daedalus {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
	};
}
