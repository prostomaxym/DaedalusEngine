#include "dlpch.h"
#include "CameraController.h"

#include "Core/Input.h"

using namespace Daedalus;

void CameraController::OnUpdate(DeltaTime ts)
{
	auto camera = m_entity.GetComponent<CameraComponent>().camera.get();

	const auto dt = ts.GetSeconds();

	const bool has_vertical_axis = KeybindConfig::FindAxisBind("MoveVertical") != nullptr;
	const bool has_horizontal_axis = KeybindConfig::FindAxisBind("MoveHorizontal") != nullptr;

	if (!has_vertical_axis)
	{
		if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveForward")))
			camera->ProcessKeyboard(CameraMovement::FORWARD, dt);

		if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveBackward")))
			camera->ProcessKeyboard(CameraMovement::BACKWARD, dt);
	}

	if (!has_horizontal_axis)
	{
		if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveLeft")))
			camera->ProcessKeyboard(CameraMovement::LEFT, dt);

		if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveRight")))
			camera->ProcessKeyboard(CameraMovement::RIGHT, dt);
	}

	if (Input::IsActionHold("MoveUp"))
		camera->ProcessKeyboard(CameraMovement::UP, dt);

	if (Input::IsActionHold("MoveDown"))
		camera->ProcessKeyboard(CameraMovement::DOWN, dt);

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
	//TODO: add zoom

	// Explicit named axes replace the corresponding raw stick and digital path.
	const auto [stick_x, stick_y] = Input::GetStickPos(Input::StickType::Left);
	const float forward = has_vertical_axis ? Input::GetAxis("MoveVertical") : -stick_y;
	const float right = has_horizontal_axis ? Input::GetAxis("MoveHorizontal") : stick_x;
	if (forward != 0.f)
		camera->MoveCamera(CameraMovement::FORWARD, forward * dt * 10.f);
	if (right != 0.f)
		camera->MoveCamera(CameraMovement::RIGHT, right * dt * 10.f);

	if (Input::IsGamepadInputEnabled())
	{
		const auto [xAxeRight, yAxeRight] = Input::GetStickPos(Input::StickType::Right);

		float xoffset = 0.f, yoffset = 0.f;

		if (xAxeRight != 0.f)
			xoffset = xAxeRight * dt * 100.f;

		if (yAxeRight != 0.f)
			yoffset  = -yAxeRight * dt * 100.f;

		camera->ProcessMouseMovement(xoffset, yoffset);
	}
}