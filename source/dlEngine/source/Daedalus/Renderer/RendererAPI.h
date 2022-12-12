#pragma once

namespace Daedalus {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1
		};
	public:
		static std::unique_ptr<RendererAPI> Create();

		virtual ~RendererAPI() = default;

		virtual void Init() = 0;

		static API GetAPI() { return s_API; }

	private:
		static API s_API;
	};

}
