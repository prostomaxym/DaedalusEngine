#include <Core/Input.h>
#include "../../dlSandbox/ExampleScripts.h"
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>

using namespace Daedalus;

namespace
{
	class TestCameraController : public CameraController
	{
	public:
		using CameraController::CameraController;
		void Tick(DeltaTime dt) { OnUpdate(dt); }
	};

	class TestPlayerController : public PlayerControllerScript
	{
	public:
		using PlayerControllerScript::PlayerControllerScript;
		void Tick(DeltaTime dt) { OnUpdate(dt); }
	};

	class FakeInput : public Input
	{
	public:
		std::array<bool, DL_KEY_MENU + 1> keys{};
		std::array<bool, DL_GAMEPAD_BUTTON_LAST + 1> buttons{};
		std::array<float, DL_GAMEPAD_AXIS_LAST + 1> axes{ 0.f, 0.f, 0.f, 0.f, -1.f, -1.f };
		bool connected{ true };
	protected:
		void UpdateImpl() override { m_previous = m_current; m_current = keys; }
		bool IsKeyPressedImpl(int code) override { return code >= 0 && code <= DL_KEY_MENU && m_current.at(code) && !m_previous.at(code); }
		bool IsKeyHoldImpl(int code) override { return code >= 0 && code <= DL_KEY_MENU && m_current.at(code) && m_previous.at(code); }
		bool IsKeyReleasedImpl(int code) override { return code >= 0 && code <= DL_KEY_MENU && !m_current.at(code) && m_previous.at(code); }
		bool IsMouseButtonPressedImpl(int) override { return false; }
		std::pair<float, float> GetMousePosImpl() override { return {}; }
		bool IsGpadKeyPressedImpl(int code) override { return connected && buttons.at(code); }
		std::pair<float, float> GetStickPosImpl(StickType type) override
		{
			if (!connected) return {};
			const size_t offset = type == StickType::Left ? 0 : 2;
			return { axes[offset], axes[offset + 1] };
		}
		float GetBumperPosImpl(BamperType type) override
		{
			return connected ? axes[type == BamperType::Left ? 4 : 5] : -1.f;
		}
	private:
		std::array<bool, DL_KEY_MENU + 1> m_current{};
		std::array<bool, DL_KEY_MENU + 1> m_previous{};
	};

	class InputActionsTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_TRUE(KeybindConfig::SetActionBind("input_jump", { DL_KEY_SPACE, DL_GAMEPAD_BUTTON_A }));
			ASSERT_TRUE(KeybindConfig::SetActionBind("input_left", { DL_KEY_A, -1 }));
			ASSERT_TRUE(KeybindConfig::SetActionBind("input_right", { DL_KEY_D, -1 }));
			ASSERT_TRUE(KeybindConfig::SetAxisBind("input_move", { "input_left", "input_right", GamepadAxis::LeftX, 1.f, 0.2f }));
			Input::EnableGamepadInput(true);
			Input::Update();
		}
		FakeInput m_input;
	};
}

TEST_F(InputActionsTest, KeyboardTransitionsRemainStableWithinFrame)
{
	m_input.keys[DL_KEY_SPACE] = true;
	Input::Update();
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
	EXPECT_FALSE(Input::IsActionHold("input_jump"));
	Input::Update();
	EXPECT_FALSE(Input::IsActionPressed("input_jump"));
	EXPECT_TRUE(Input::IsActionHold("input_jump"));
	m_input.keys[DL_KEY_SPACE] = false;
	Input::Update();
	EXPECT_FALSE(Input::IsActionDown("input_jump"));
	EXPECT_TRUE(Input::IsActionReleased("input_jump"));
	Input::Update();
	EXPECT_FALSE(Input::IsActionReleased("input_jump"));
}

TEST_F(InputActionsTest, KeyboardAndGamepadShareOneTransition)
{
	m_input.buttons[DL_GAMEPAD_BUTTON_A] = true;
	Input::Update();
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
	m_input.keys[DL_KEY_SPACE] = true;
	Input::Update();
	EXPECT_TRUE(Input::IsActionHold("input_jump"));
	EXPECT_FALSE(Input::IsActionPressed("input_jump"));
	m_input.buttons[DL_GAMEPAD_BUTTON_A] = false;
	Input::Update();
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
	EXPECT_FALSE(Input::IsActionReleased("input_jump"));
	m_input.keys[DL_KEY_SPACE] = false;
	Input::Update();
	EXPECT_TRUE(Input::IsActionReleased("input_jump"));
}

TEST_F(InputActionsTest, DisconnectReleasesGamepadAndNeutralizesAxes)
{
	m_input.buttons[0] = true;
	m_input.axes[0] = 1.f;
	Input::Update();
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 1.f);
	m_input.connected = false;
	Input::Update();
	EXPECT_TRUE(Input::IsActionReleased("input_jump"));
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 0.f);
	Input::Update();
	EXPECT_FALSE(Input::IsActionReleased("input_jump"));
}

TEST_F(InputActionsTest, DisableGatesApplyImmediatelyWithoutSuppressingOtherDevice)
{
	m_input.keys[DL_KEY_SPACE] = true;
	m_input.keys[DL_KEY_D] = true;
	m_input.buttons[0] = true;
	m_input.axes[0] = 1.f;
	Input::Update();
	Input::Update();
	Input::EnableKeyboardInput(false);
	EXPECT_FALSE(Input::IsKeyHold(DL_KEY_SPACE));
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 1.f);
	Input::EnableGamepadInput(false);
	EXPECT_FALSE(Input::IsActionDown("input_jump"));
	EXPECT_TRUE(Input::IsActionReleased("input_jump"));
	EXPECT_FALSE(Input::IsGpadKeyPressed(0));
	EXPECT_FLOAT_EQ(Input::GetStickPos(Input::StickType::Left).first, 0.f);
	EXPECT_FLOAT_EQ(Input::GetBumperPos(Input::BamperType::Left), 0.f);
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 0.f);
	Input::Update();
	EXPECT_FALSE(Input::IsActionDown("input_jump"));
	Input::EnableKeyboardInput(true);
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
}

TEST_F(InputActionsTest, RebindingEstablishesBaselineOnNextUpdate)
{
	m_input.keys[DL_KEY_SPACE] = true;
	Input::Update();
	ASSERT_TRUE(Input::IsActionPressed("input_jump"));
	ASSERT_TRUE(KeybindConfig::SetActionBind("input_jump", { DL_KEY_J, -1 }));
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
	Input::Update();
	EXPECT_FALSE(Input::IsActionDown("input_jump"));
	EXPECT_FALSE(Input::IsActionReleased("input_jump"));
	m_input.keys[DL_KEY_J] = true;
	Input::Update();
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
	ASSERT_TRUE(KeybindConfig::SetActionBind("input_jump", { DL_KEY_SPACE, -1 }));
	Input::Update();
	EXPECT_TRUE(Input::IsActionDown("input_jump"));
	EXPECT_FALSE(Input::IsActionPressed("input_jump"));
}

TEST_F(InputActionsTest, DigitalAxesCancelOpposingDirectionsAndClampCombinedInput)
{
	m_input.keys[DL_KEY_A] = true;
	Input::Update();
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), -1.f);
	m_input.keys[DL_KEY_D] = true;
	Input::Update();
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 0.f);
	m_input.keys[DL_KEY_A] = false;
	m_input.axes[0] = 0.6f;
	Input::Update();
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 1.f);
	m_input.axes[0] = -0.6f;
	EXPECT_NEAR(Input::GetAxis("input_move"), 0.5f, 0.00001f);
}

TEST_F(InputActionsTest, AnalogDeadzoneIsRescaledAndScaleInvertsOnlyAnalog)
{
	for (const float value : { -0.2f, 0.f, 0.2f })
	{
		m_input.axes[0] = value;
		EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 0.f);
	}
	m_input.axes[0] = 0.6f;
	EXPECT_NEAR(Input::GetAxis("input_move"), 0.5f, 0.00001f);
	m_input.axes[0] = -0.6f;
	EXPECT_NEAR(Input::GetAxis("input_move"), -0.5f, 0.00001f);
	ASSERT_TRUE(KeybindConfig::SetAxisBind("input_move", { "input_left", "input_right", GamepadAxis::LeftX, -1.f, 0.2f }));
	EXPECT_NEAR(Input::GetAxis("input_move"), 0.5f, 0.00001f);
	m_input.axes[0] = 0.f;
	m_input.keys[DL_KEY_D] = true;
	Input::Update();
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 1.f);
}

TEST_F(InputActionsTest, AllGamepadAxesAndTriggerNormalizationAreSupported)
{
	for (int code = 0; code <= DL_GAMEPAD_AXIS_LAST; ++code)
	{
		ASSERT_TRUE(KeybindConfig::SetAxisBind("input_analog", { {}, {}, static_cast<GamepadAxis>(code), 1.f, 0.f }));
		m_input.axes[code] = code < 4 ? -0.5f : 0.f;
		EXPECT_FLOAT_EQ(Input::GetAxis("input_analog"), code < 4 ? -0.5f : 0.5f);
		m_input.axes[code] = 1.f;
		EXPECT_FLOAT_EQ(Input::GetAxis("input_analog"), 1.f);
		m_input.connected = false;
		EXPECT_FLOAT_EQ(Input::GetAxis("input_analog"), 0.f);
		m_input.connected = true;
	}
}

TEST_F(InputActionsTest, UnknownNamesAndUnboundMappingsAreNeutral)
{
	EXPECT_FALSE(Input::IsActionDown("missing_input_action"));
	EXPECT_FALSE(Input::IsActionPressed("missing_input_action"));
	EXPECT_FALSE(Input::IsActionHold("missing_input_action"));
	EXPECT_FALSE(Input::IsActionReleased("missing_input_action"));
	EXPECT_FLOAT_EQ(Input::GetAxis("missing_input_axis"), 0.f);
	ASSERT_TRUE(KeybindConfig::SetActionBind("input_unbound", {}));
	ASSERT_TRUE(KeybindConfig::SetAxisBind("input_empty_axis", {}));
	Input::Update();
	EXPECT_FALSE(Input::IsActionDown("input_unbound"));
	EXPECT_FLOAT_EQ(Input::GetAxis("input_empty_axis"), 0.f);
	EXPECT_FALSE(Input::IsGpadKeyPressed(-1));
	EXPECT_FALSE(Input::IsGpadKeyPressed(15));
}

TEST_F(InputActionsTest, NonfiniteAnalogInputIsNeutralAndOutOfRangeIsClamped)
{
	for (const float value : { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
	{
		m_input.axes[0] = value;
		EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 0.f);
	}
	m_input.axes[0] = 2.f;
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), 1.f);
	m_input.axes[0] = -2.f;
	EXPECT_FLOAT_EQ(Input::GetAxis("input_move"), -1.f);
}

TEST_F(InputActionsTest, InvalidStickDeadzonesDoNotReplaceLastValidValue)
{
	Input::SetSticksDeadZone(0.3f);
	m_input.axes[0] = 0.2f;
	for (const float value : { -1.f, 1.f, std::numeric_limits<float>::quiet_NaN() })
	{
		Input::SetSticksDeadZone(value);
		EXPECT_FLOAT_EQ(Input::GetStickPos(Input::StickType::Left).first, 0.f);
	}
}

TEST_F(InputActionsTest, NativeCameraIgnoresPlanarActionButtonsButPreservesVerticalButtons)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	TestCameraController script(entity);
	const DeltaTime dt(std::chrono::milliseconds(100));
	const char* names[] = { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" };
	const CameraMovement directions[] = { CameraMovement::FORWARD, CameraMovement::BACKWARD, CameraMovement::LEFT,
		CameraMovement::RIGHT, CameraMovement::UP, CameraMovement::DOWN };
	for (int i = 0; i < 6; ++i)
		ASSERT_TRUE(KeybindConfig::SetActionBind(names[i], { -1, i }));
	Input::Update();

	for (int i = 0; i < 6; ++i)
	{
		SCOPED_TRACE(names[i]);
		camera.SetPosition(glm::vec3(0.f));
		PerspectiveCamera expected;
		if (i >= 4) expected.ProcessKeyboard(directions[i], dt.GetSeconds());
		m_input.buttons[i] = true;
		Input::Update();
		script.Tick(dt);
		EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
		Input::Update();
		script.Tick(dt);
		EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
		m_input.buttons[i] = false;
		Input::Update();
		script.Tick(dt);
		EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
	}
}

TEST_F(InputActionsTest, ChangePovButtonDoesNotMoveCameraOrPlayerWithSharedMovementBindings)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, DL_GAMEPAD_BUTTON_B }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveLeft", { DL_KEY_A, DL_GAMEPAD_BUTTON_A }));
	for (const auto* name : { "MoveBackward", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	entity.AddComponent<RenderableObjectComponent>();
	entity.AddComponent<SpotLightComponent>();
	auto& transform = entity.GetComponent<TransformComponent>();
	transform.rotation.y = 17.f;
	TestCameraController camera_script(entity);
	TestPlayerController player_script(entity);
	const DeltaTime dt(std::chrono::milliseconds(100));
	for (const auto button : { DL_GAMEPAD_BUTTON_B, DL_GAMEPAD_BUTTON_A })
	{
		m_input.buttons[button] = true;
		for (int frame = 0; frame < 3; ++frame)
		{
			Input::Update();
			EXPECT_TRUE(Input::IsActionDown(button == DL_GAMEPAD_BUTTON_B ? "MoveForward" : "MoveLeft"));
			camera_script.Tick(dt);
			player_script.Tick(dt);
			EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
			EXPECT_EQ(transform.translation, glm::vec3(0.f));
			EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
		}
		m_input.buttons[button] = false;
		Input::Update();
		camera_script.Tick(dt);
		player_script.Tick(dt);
		EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
		EXPECT_EQ(transform.translation, glm::vec3(0.f));
		EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
		EXPECT_FALSE(entity.HasComponent<RenderableObjectComponent>());
		EXPECT_TRUE(entity.HasComponent<SpotLightComponent>());
	}
	EXPECT_EQ(KeybindConfig::FindActionBind("MoveForward")->gamepad_button, DL_GAMEPAD_BUTTON_B);
	EXPECT_EQ(KeybindConfig::FindActionBind("ChangePOV")->gamepad_button, DL_GAMEPAD_BUTTON_B);
	// An already-held gamepad action must not bypass the keyboard's first-frame delay.
	m_input.buttons[DL_GAMEPAD_BUTTON_B] = true;
	Input::Update();
	Input::Update();
	m_input.keys[DL_KEY_W] = true;
	Input::Update();
	EXPECT_TRUE(Input::IsActionHold("MoveForward"));
	camera_script.Tick(dt);
	player_script.Tick(dt);
	EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
	EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
	Input::Update();
	camera_script.Tick(dt);
	PerspectiveCamera expected;
	expected.ProcessKeyboard(CameraMovement::FORWARD, dt.GetSeconds());
	EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
}

TEST_F(InputActionsTest, NativeCameraPreservesKeyboardHoldAndRawStickMovement)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	TestCameraController script(entity);
	const DeltaTime dt(std::chrono::milliseconds(100));
	m_input.keys[DL_KEY_W] = true;
	Input::Update();
	script.Tick(dt);
	EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
	Input::Update();
	script.Tick(dt);
	PerspectiveCamera expected;
	expected.ProcessKeyboard(CameraMovement::FORWARD, dt.GetSeconds());
	EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
	m_input.keys[DL_KEY_W] = false;
	m_input.axes[0] = 1.f;
	Input::Update();
	script.Tick(dt);
	expected.MoveCamera(CameraMovement::RIGHT, dt.GetSeconds() * 10.f);
	EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
}

TEST_F(InputActionsTest, ExamplePlayerGamepadTogglesOnlyOnReleaseAndRestoresComponents)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	entity.AddComponent<CameraComponent>(CameraProjectionProps{});
	entity.AddComponent<RenderableObjectComponent>();
	entity.AddComponent<SpotLightComponent>();
	TestPlayerController script(entity);

	for (int toggle = 0; toggle < 2; ++toggle)
	{
		const bool initially_present = toggle == 0;
		m_input.buttons[DL_GAMEPAD_BUTTON_Y] = true;
		m_input.buttons[DL_GAMEPAD_BUTTON_B] = true;
		for (int frame = 0; frame < 3; ++frame)
		{
			Input::Update();
			script.Tick({});
			EXPECT_EQ(entity.HasComponent<SpotLightComponent>(), initially_present);
			EXPECT_EQ(entity.HasComponent<RenderableObjectComponent>(), initially_present);
		}
		m_input.buttons[DL_GAMEPAD_BUTTON_Y] = false;
		m_input.buttons[DL_GAMEPAD_BUTTON_B] = false;
		Input::Update();
		script.Tick({});
		EXPECT_EQ(entity.HasComponent<SpotLightComponent>(), !initially_present);
		EXPECT_EQ(entity.HasComponent<RenderableObjectComponent>(), !initially_present);
		Input::Update();
		script.Tick({});
		EXPECT_EQ(entity.HasComponent<SpotLightComponent>(), !initially_present);
		EXPECT_EQ(entity.HasComponent<RenderableObjectComponent>(), !initially_present);
	}
}

TEST_F(InputActionsTest, ExamplePlayerWaitsForBothDevicesToReleaseAndUsesStickForFacing)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
	for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	entity.AddComponent<CameraComponent>(CameraProjectionProps{}, CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 0.f));
	entity.AddComponent<RenderableObjectComponent>();
	entity.AddComponent<SpotLightComponent>();
	TestPlayerController script(entity);
	m_input.keys[DL_KEY_F] = true;
	m_input.keys[DL_KEY_V] = true;
	m_input.buttons[DL_GAMEPAD_BUTTON_Y] = true;
	m_input.buttons[DL_GAMEPAD_BUTTON_B] = true;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = -0.5f;
	Input::Update();
	script.Tick({});
	m_input.buttons[DL_GAMEPAD_BUTTON_Y] = false;
	m_input.buttons[DL_GAMEPAD_BUTTON_B] = false;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::milliseconds(100)));
	EXPECT_TRUE(entity.HasComponent<SpotLightComponent>());
	EXPECT_TRUE(entity.HasComponent<RenderableObjectComponent>());
	EXPECT_GT(entity.GetComponent<TransformComponent>().rotation.y, 0.f);
	m_input.keys[DL_KEY_F] = false;
	m_input.keys[DL_KEY_V] = false;
	Input::Update();
	script.Tick({});
	EXPECT_FALSE(entity.HasComponent<SpotLightComponent>());
	EXPECT_FALSE(entity.HasComponent<RenderableObjectComponent>());
}

TEST_F(InputActionsTest, NativeCameraPreservesSignedStickMagnitudeAndAnalogLook)
{
	for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	TestCameraController script(entity);
	const DeltaTime dt(std::chrono::milliseconds(100));
	const std::array<std::pair<float, float>, 7> positions{
		std::pair{ 0.f, -0.5f }, { 0.f, 0.5f }, { -0.5f, 0.f }, { 0.5f, 0.f },
		{ 0.3f, -0.7f }, { -0.3f, 0.7f }, { 0.1f, -0.1f } };
	for (const auto& [x, y] : positions)
	{
		SCOPED_TRACE(x);
		SCOPED_TRACE(y);
		camera.SetPosition(glm::vec3(0.f));
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = x;
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = y;
		Input::Update();
		PerspectiveCamera expected;
		const auto [filtered_x, filtered_y] = Input::GetStickPos(Input::StickType::Left);
		expected.MoveCamera(CameraMovement::FORWARD, -filtered_y * dt.GetSeconds() * 10.f);
		expected.MoveCamera(CameraMovement::RIGHT, filtered_x * dt.GetSeconds() * 10.f);
		script.Tick(dt);
		EXPECT_EQ(camera.GetPosition(), expected.GetPosition());
	}
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.f;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = 0.f;
	m_input.axes[DL_GAMEPAD_AXIS_RIGHT_X] = 0.5f;
	m_input.axes[DL_GAMEPAD_AXIS_RIGHT_Y] = -0.25f;
	PerspectiveCamera expected;
	expected.ProcessMouseMovement(0.5f * dt.GetSeconds() * 100.f, 0.25f * dt.GetSeconds() * 100.f);
	Input::Update();
	script.Tick(dt);
	EXPECT_FLOAT_EQ(camera.GetYaw(), expected.GetYaw());
	EXPECT_FLOAT_EQ(camera.GetPitch(), expected.GetPitch());
	const auto yaw = camera.GetYaw();
	Input::EnableGamepadInput(false);
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 1.f;
	script.Tick(dt);
	EXPECT_EQ(camera.GetPosition(), glm::vec3(0.f));
	EXPECT_FLOAT_EQ(camera.GetYaw(), yaw);
}

TEST_F(InputActionsTest, ExamplePlayerFacesSignedStickDirectionAndIgnoresNeutralOrDisconnectedStick)
{
	for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "Flashlight", "ChangePOV" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	entity.AddComponent<SpotLightComponent>();
	TestPlayerController script(entity);
	auto& transform = entity.GetComponent<TransformComponent>();
	const std::array<std::pair<float, float>, 6> positions{
		std::pair{ 0.f, -0.5f }, { 0.f, 0.5f }, { -0.5f, 0.f }, { 0.5f, 0.f },
		{ 0.3f, -0.7f }, { -0.3f, 0.7f } };
	for (const float pitch : { 0.f, 60.f, -60.f })
	{
		camera.SetRotation(-45.f, pitch);
		for (const auto& [x, y] : positions)
		{
			transform.rotation.y = 0.f;
			m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = x;
			m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = y;
			Input::Update();
			script.Tick(DeltaTime(std::chrono::seconds(1)));
			const auto direction = (camera.GetDirection() * -y + camera.GetRight() * x) * 10.f;
			float expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
			if (expected_yaw < 0.f) expected_yaw += 360.f;
			EXPECT_NEAR(transform.rotation.y, expected_yaw, 0.0001f);
		}
	}
	transform.rotation.y = 17.f;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.1f;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = -0.1f;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 1.f;
	Input::EnableGamepadInput(false);
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
	Input::EnableGamepadInput(true);
	m_input.connected = false;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
}

TEST_F(InputActionsTest, ExamplePlayerCombinesKeyboardHoldWithAnalogCameraSpeed)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "Flashlight", "ChangePOV" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	entity.AddComponent<SpotLightComponent>();
	TestPlayerController script(entity);
	auto& transform = entity.GetComponent<TransformComponent>();
	transform.rotation.y = 17.f;
	m_input.keys[DL_KEY_W] = true;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_FLOAT_EQ(transform.rotation.y, 17.f);
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.5f;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	const auto direction = camera.GetDirection() * camera.GetMovementSpeed() + camera.GetRight() * 5.f;
	float expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
	if (expected_yaw < 0.f) expected_yaw += 360.f;
	EXPECT_NEAR(transform.rotation.y, expected_yaw, 0.0001f);
}

TEST_F(InputActionsTest, NamedMovementAxesReplaceRawComponentsAndRespectBindings)
{
	// Isolate opt-in bindings: the process-wide Config has no axis-removal API.
	ASSERT_EXIT(([&]()
	{
		ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
		for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown", "Flashlight", "ChangePOV" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveVertical", { "MoveBackward", "MoveForward", GamepadAxis::LeftY, -1.f, 0.2f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
		entity.AddComponent<SpotLightComponent>();
		TestCameraController camera_script(entity);
		TestPlayerController player_script(entity);
		const DeltaTime dt(std::chrono::milliseconds(100));
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.4f;
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = -0.6f;
		Input::Update();
		PerspectiveCamera expected;
		expected.MoveCamera(CameraMovement::FORWARD, 0.5f * dt.GetSeconds() * 10.f);
		expected.MoveCamera(CameraMovement::RIGHT, 0.4f * dt.GetSeconds() * 10.f);
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveHorizontal", { "MoveLeft", "MoveRight", GamepadAxis::RightX, -1.f, 0.f }));
		m_input.axes[DL_GAMEPAD_AXIS_RIGHT_X] = 0.25f;
		camera.SetPosition(glm::vec3(0.f));
		expected.SetPosition(glm::vec3(0.f));
		expected.MoveCamera(CameraMovement::FORWARD, 0.5f * dt.GetSeconds() * 10.f);
		expected.MoveCamera(CameraMovement::RIGHT, -0.25f * dt.GetSeconds() * 10.f);
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		player_script.Tick(DeltaTime(std::chrono::seconds(1)));
		const auto direction = camera.GetDirection() * 0.5f - camera.GetRight() * 0.25f;
		float expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
		if (expected_yaw < 0.f) expected_yaw += 360.f;
		EXPECT_NEAR(entity.GetComponent<TransformComponent>().rotation.y, expected_yaw, 0.0001f);
		Input::EnableGamepadInput(false);
		m_input.keys[DL_KEY_W] = true;
		Input::Update();
		camera.SetPosition(glm::vec3(0.f));
		expected.SetPosition(glm::vec3(0.f));
		expected.SetRotation(camera.GetYaw(), camera.GetPitch());
		expected.MoveCamera(CameraMovement::FORWARD, dt.GetSeconds() * 10.f);
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(InputActionsTest, ConnectionChangesDoNotOverrideExplicitGamepadGate)
{
	Input::EnableGamepadInput(false);
	m_input.buttons[DL_GAMEPAD_BUTTON_A] = true;
	for (const bool connected : { false, true, false, true })
	{
		m_input.connected = connected;
		Input::Update();
		EXPECT_FALSE(Input::IsGamepadInputEnabled());
		EXPECT_FALSE(Input::IsActionDown("input_jump"));
		EXPECT_FALSE(Input::IsGpadKeyPressed(DL_GAMEPAD_BUTTON_A));
	}
	Input::EnableGamepadInput(true);
	m_input.connected = false;
	Input::Update();
	EXPECT_TRUE(Input::IsGamepadInputEnabled());
	EXPECT_FALSE(Input::IsActionDown("input_jump"));
	m_input.connected = true;
	Input::Update();
	EXPECT_TRUE(Input::IsGamepadInputEnabled());
	EXPECT_TRUE(Input::IsActionPressed("input_jump"));
}
