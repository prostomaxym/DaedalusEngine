#include "GameApp.h"

#include "GameLayer.h"
using namespace Daedalus;

Application* Daedalus::CreateApplication()
{
	// Entry point is defined in engine and takes ownership of the returned application
	return new GameApp();
}

GameApp::GameApp()
{
	PushLayer(std::make_unique<GameLayer>());
}
