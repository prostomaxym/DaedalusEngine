#include <Debug/Log.h>
#include <Config/GraphicsConfig.h>
#include <Config/PathConfig.h>
#include <Config/KeybindConfig.h>
#include <Renderer/Resources/ResourceManager.h>
#include <Physics/Physics.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	Daedalus::PathConfig paths;
	Daedalus::GraphicsConfig graphics;
	Daedalus::KeybindConfig keybinds;
	Daedalus::ResourceManager resources;
	Daedalus::Physics physics;
	Daedalus::Log::Init();
	const auto result = RUN_ALL_TESTS();
	Daedalus::Log::Shutdown();
	return result;
}