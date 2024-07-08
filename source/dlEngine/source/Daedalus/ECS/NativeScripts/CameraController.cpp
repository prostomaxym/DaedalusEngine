#include "dlpch.h"
#include "CameraController.h"

#include "Daedalus/Core/Input.h"

using namespace Daedalus;

void CameraController::OnUpdate(DeltaTime ts)
{
	auto& camera = m_entity.GetComponent<CameraComponent>().camera;

	const auto dt = ts.GetSeconds();

	if (Input::IsKeyHold(DL_KEY_W))
		camera.ProcessKeyboard(CameraMovement::FORWARD, dt);

	if (Input::IsKeyHold(DL_KEY_S))
		camera.ProcessKeyboard(CameraMovement::BACKWARD, dt);

	if (Input::IsKeyHold(DL_KEY_A))
		camera.ProcessKeyboard(CameraMovement::LEFT, dt);

	if (Input::IsKeyHold(DL_KEY_D))
		camera.ProcessKeyboard(CameraMovement::RIGHT, dt);

	if (Input::IsKeyHold(DL_KEY_SPACE))
		camera.ProcessKeyboard(CameraMovement::UP, dt);

	if (Input::IsKeyHold(DL_KEY_C))
		camera.ProcessKeyboard(CameraMovement::DOWN, dt);

	if (Input::IsKeyHold(DL_KEY_C))
		camera.ProcessKeyboard(CameraMovement::DOWN, dt);

	UpdateSpotlight();

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
	//TODO: add zoom

	if (Input::IsGamepadInputEnabled())
	{
		const auto [xAxeLeft, yAxeLeft] = Input::GetStickPos(Input::StickType::Left);

		if (yAxeLeft != 0.f)
			camera.MoveCamera(CameraMovement::FORWARD, -yAxeLeft * dt * 10.f);

		if (xAxeLeft != 0.f)
			camera.MoveCamera(CameraMovement::RIGHT, xAxeLeft * dt * 10.f);

		const auto [xAxeRight, yAxeRight] = Input::GetStickPos(Input::StickType::Right);

		float xoffset = 0.f, yoffset = 0.f;

		if (xAxeRight != 0.f)
			xoffset = xAxeRight * dt * 100.f;

		if (yAxeRight != 0.f)
			yoffset  = -yAxeRight * dt * 100.f;

		camera.ProcessMouseMovement(xoffset, yoffset);
	}
}

void CameraController::UpdateSpotlight()
{
	const auto key_pressed = Input::IsKeyReleased(DL_KEY_F);

	if (key_pressed && m_spotlight_enabled)
	{
		m_spotlight_enabled = false;
		m_entity.RemoveComponent<SpotLightComponent>();
	}
	else if (key_pressed && !m_spotlight_enabled)
	{
		m_spotlight_enabled = true;
		m_entity.AddComponent<SpotLightComponent>(glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f), true, 2.f, 500.f, glm::vec3{ 0.f,0.f,1.f }, 10.f, 35.f, true);
	}
}