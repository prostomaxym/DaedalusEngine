#include "dlpch.h"
#include "RenderCommand.h"

namespace Daedalus {

	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}