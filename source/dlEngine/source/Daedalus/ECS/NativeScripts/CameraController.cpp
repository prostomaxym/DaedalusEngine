#include "dlpch.h"
#include "CameraController.h"

#include "Daedalus/Core/Input.h"

using namespace Daedalus;

void CameraController::OnUpdate(DeltaTime ts)
{
	auto& camera = m_entity.GetComponent<CameraComponent>().camera;

	const auto dt = ts.GetSeconds();

	if (Input::IsKeyPressed(DL_KEY_W))
		camera.ProcessKeyboard(CameraMovement::FORWARD, dt);

	if (Input::IsKeyPressed(DL_KEY_S))
		camera.ProcessKeyboard(CameraMovement::BACKWARD, dt);

	if (Input::IsKeyPressed(DL_KEY_A))
		camera.ProcessKeyboard(CameraMovement::LEFT, dt);

	if (Input::IsKeyPressed(DL_KEY_D))
		camera.ProcessKeyboard(CameraMovement::RIGHT, dt);

	if (Input::IsKeyPressed(DL_KEY_SPACE))
		camera.ProcessKeyboard(CameraMovement::UP, dt);

	if (Input::IsKeyPressed(DL_KEY_C))
		camera.ProcessKeyboard(CameraMovement::DOWN, dt);


	const auto [xCurrent, yCurrent] = Input::GetMousePos();

	if (!xPrevious.has_value() || !yPrevious.has_value())
	{
		xPrevious = xCurrent;
		yPrevious = yCurrent;
	}

	const auto dx = xCurrent - xPrevious.value();
	const auto dy = yPrevious.value() - yCurrent;

	xPrevious = xCurrent;
	yPrevious = yCurrent;

	camera.ProcessMouseMovement(dx * dt * 2.f, dy * dt * 2.f);
	camera.ProcessZoom(Input::);

	const auto pos = camera.GetPosition();
	const auto message = std::string("Position - ") + "X: " + std::to_string(pos.x) + " / " + "Y: " + std::to_string(pos.y) + " / " + "Z: " + std::to_string(pos.z);
	Log::Write(Log::Levels::Trace, Log::Categories::Application, message);
}

