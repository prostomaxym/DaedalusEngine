#pragma once

#include <Daedalus.h>

namespace Daedalus 
{
	class ExampleLayer final : public Layer
	{
	public:
		explicit ExampleLayer(std::filesystem::path scene_file = "scenes/nuke.scene.json")
			: Layer("Example", true), m_scene_file(std::move(scene_file)) {}

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(DeltaTime dt) override;

	private:
		std::filesystem::path m_scene_file;
		ScriptRegistry m_scripts;
	};
}