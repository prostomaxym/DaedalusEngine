#include "dlpch.h"
#include "CameraController.h"

#include "Core/Input.h"

using namespace Daedalus;

void CameraController::OnUpdate(DeltaTime ts)
{
	const auto dt = ts.GetSeconds();
	ProcessMovement(dt);
	ProcessMouse(dt);
	ProcessGamepad(dt);
}

void CameraController::ProcessMovement(float dt)
{
	auto camera = m_entity.GetComponent<CameraComponent>().camera.get();

	const auto forward = Input::GetMovementAction("MoveForward", "MoveBackward");
	if (forward != 0.f)
		camera->ProcessKeyboard(CameraMovement::FORWARD, forward * dt);

	const auto right = Input::GetMovementAction("MoveRight", "MoveLeft");
	if (right != 0.f)
		camera->ProcessKeyboard(CameraMovement::RIGHT, right * dt);

	if (Input::IsActionHold("MoveUp"))
		camera->ProcessKeyboard(CameraMovement::UP, dt);

	if (Input::IsActionHold("MoveDown"))
		camera->ProcessKeyboard(CameraMovement::DOWN, dt);
}

void CameraController::ProcessMouse(float dt)
{
	auto camera = m_entity.GetComponent<CameraComponent>().camera.get();

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

	camera->ProcessMouseMovement(dx * dt * 2.f, dy * dt * 2.f);
}

void CameraController::ProcessGamepad(float dt)
{
	if (Input::IsGamepadInputEnabled())
	{
		auto camera = m_entity.GetComponent<CameraComponent>().camera.get();
		const auto x = Input::GetAxis("LookRight");
		const auto y = Input::GetAxis("LookUp");

		float xoffset = 0.f, yoffset = 0.f;

		if (x != 0.f)
			xoffset = x * dt * 100.f;

		if (y != 0.f)
			yoffset  = -y * dt * 100.f;

		camera->ProcessMouseMovement(xoffset, yoffset);
	}
}