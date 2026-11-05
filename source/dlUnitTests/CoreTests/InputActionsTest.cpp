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
	Camera& AddLinkedCamera(Entity owner)
	{
		auto entity = owner.GetScene().CreateEntity();
		auto& camera = *entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
		entity.GetComponent<TransformComponent>().SetOrientationQuat(camera.GetOrientation());
		EXPECT_TRUE(owner.GetScene().SetEntityLink(owner.GetUUID(), "camera", entity.GetUUID()));
		owner.GetScene().UpdateWorldTransforms();
		return camera;
	}

	void ResetCameraPosition(Entity owner)
	{
		owner.GetLinkedEntity("camera").GetComponent<TransformComponent>().translation = glm::vec3(0.f);
		owner.GetScene().UpdateWorldTransforms();
	}

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

TEST_F(InputActionsTest, camera_controller_writes_local_trs_under_rotated_parent)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::EnableGamepadInput(false);
	Scene scene;
	auto owner = scene.CreateEntity();
	auto parent = scene.CreateEntity();
	auto& camera = AddLinkedCamera(owner);
	auto entity = owner.GetLinkedEntity("camera");
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	ASSERT_TRUE(scene.SetParent(entity.GetUUID(), parent.GetUUID()));
	TestCameraController script(owner);
	m_input.keys[DL_KEY_W] = true;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_NEAR(entity.GetComponent<TransformComponent>().translation.x, camera.GetMovementSpeed(), 0.0001f);
	EXPECT_NEAR(camera.GetPosition().x, 10.f, 0.0001f);
	EXPECT_NEAR(camera.GetPosition().z, -camera.GetMovementSpeed(), 0.0001f);
}

TEST_F(InputActionsTest, camera_controller_preserves_roll_without_look_input)
{
	for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::EnableGamepadInput(false);
	Scene scene;
	auto owner = scene.CreateEntity();
	AddLinkedCamera(owner);
	auto entity = owner.GetLinkedEntity("camera");
	auto& transform = entity.GetComponent<TransformComponent>();
	transform.rotation = { 10.f, 20.f, 30.f };
	const auto expected = transform.GetOrientationQuat();
	TestCameraController script(owner);
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_NEAR(std::abs(glm::dot(expected, transform.GetOrientationQuat())), 1.f, 0.0001f);
}

TEST_F(InputActionsTest, player_facing_converts_world_movement_to_parent_space)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "Flashlight", "ChangePOV" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::EnableGamepadInput(false);
	Scene scene;
	auto entity = scene.CreateEntity();
	auto parent = scene.CreateEntity();
	AddLinkedCamera(entity);
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	ASSERT_TRUE(scene.SetParent(entity.GetUUID(), parent.GetUUID()));
	TestPlayerController script(entity);
	m_input.keys[DL_KEY_W] = true;
	Input::Update();
	script.Tick(DeltaTime(std::chrono::seconds(1)));
	EXPECT_NEAR(entity.GetComponent<TransformComponent>().rotation.y, 0.f, 0.0001f);
	const auto front = scene.GetWorldOrientation(entity.GetUUID()) * glm::vec3(0.f, 0.f, 1.f);
	EXPECT_NEAR(front.x, 1.f, 0.0001f);
	EXPECT_NEAR(front.z, 0.f, 0.0001f);
}

TEST_F(InputActionsTest, player_facing_survives_world_pose_euler_writeback_across_frames)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveBackward", { DL_KEY_S, -1 }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveLeft", { DL_KEY_A, -1 }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveRight", { DL_KEY_D, -1 }));
	for (const auto* name : { "MoveUp", "MoveDown", "Flashlight", "ChangePOV" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::EnableGamepadInput(false);
	const DeltaTime dt(std::chrono::milliseconds(16));
	for (const float parent_yaw : { 0.f, 90.f })
	{
		SCOPED_TRACE(parent_yaw);
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = AddLinkedCamera(entity);
		auto camera_entity = entity.GetLinkedEntity("camera");
		camera_entity.GetComponent<TransformComponent>().rotation = glm::vec3(0.f);
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.rotation = { 0.f, 180.f, 0.f };
		transform.scale = glm::vec3(2.f);
		if (parent_yaw != 0.f)
		{
			auto parent = scene.CreateEntity();
			parent.GetComponent<TransformComponent>().rotation.y = parent_yaw;
			ASSERT_TRUE(scene.SetParent(entity.GetUUID(), parent.GetUUID()));
		}
		auto& body = entity.AddComponent<KinematicBodyComponent>();
		scene.UpdateWorldTransforms();
		TestCameraController camera_script(entity);
		TestPlayerController player_script(entity);
		for (const int key : { DL_KEY_W, DL_KEY_D, DL_KEY_S, DL_KEY_A })
		{
			SCOPED_TRACE(key);
			m_input.keys.fill(false);
			m_input.keys[key] = true;
			for (int frame = 0; frame < 90; ++frame)
			{
				Input::Update();
				const auto previous = scene.GetWorldOrientation(entity.GetUUID()) * glm::vec3(0.f, 0.f, 1.f);
				camera_script.Tick(dt);
				player_script.Tick(dt);
				scene.StepPhysics(dt);
				const auto facing = scene.GetWorldOrientation(entity.GetUUID()) * glm::vec3(0.f, 0.f, 1.f);
				EXPECT_GE(glm::dot(previous, facing), std::cos(glm::radians(540.f * dt.GetSeconds())) - 0.0001f);
				EXPECT_NEAR(facing.y, 0.f, 0.0001f);
			}
			const auto expected = glm::normalize(camera.GetDirection() * Input::GetMovementAction("MoveForward", "MoveBackward")
				+ camera.GetRight() * Input::GetMovementAction("MoveRight", "MoveLeft"));
			const auto facing = scene.GetWorldOrientation(entity.GetUUID()) * glm::vec3(0.f, 0.f, 1.f);
			EXPECT_NEAR(glm::distance(facing, expected), 0.f, 0.0001f);
			EXPECT_NEAR(glm::distance(glm::vec3((*scene.GetWorldTransform(entity.GetUUID()))[3]), camera.GetPosition()), 0.f, 0.0001f);
			glm::vec3 body_position;
			glm::quat body_orientation;
			ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(body.handle, body_position, body_orientation));
			EXPECT_NEAR(glm::distance(body_orientation * glm::vec3(0.f, 0.f, 1.f), expected), 0.f, 0.0001f);
			EXPECT_NEAR(glm::distance(transform.scale, glm::vec3(2.f)), 0.f, 0.0001f);
		}
		m_input.keys.fill(false);
		Input::Update();
		const auto idle_orientation = scene.GetWorldOrientation(entity.GetUUID());
		player_script.Tick(dt);
		EXPECT_NEAR(std::abs(glm::dot(idle_orientation, scene.GetWorldOrientation(entity.GetUUID()))), 1.f, 0.0001f);
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

TEST_F(InputActionsTest, camera_uses_planar_action_down_and_vertical_action_hold)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = AddLinkedCamera(entity);
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
		ResetCameraPosition(entity);
		PerspectiveCamera expected;
		m_input.buttons[i] = true;
		Input::Update();
		if (i < 4) expected.ProcessKeyboard(directions[i], dt.GetSeconds());
		script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		Input::Update();
		expected.ProcessKeyboard(directions[i], dt.GetSeconds());
		script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		m_input.buttons[i] = false;
		Input::Update();
		script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
	}
}

TEST_F(InputActionsTest, shared_movement_and_pov_button_moves_on_down_and_toggles_on_release)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, DL_GAMEPAD_BUTTON_B }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = AddLinkedCamera(entity);
	entity.AddComponent<RenderableObjectComponent>();
	entity.AddComponent<SpotLightComponent>();
	TestCameraController camera_script(entity);
	TestPlayerController player_script(entity);
	const DeltaTime dt(std::chrono::seconds(1));
	PerspectiveCamera expected;
	m_input.buttons[DL_GAMEPAD_BUTTON_B] = true;
	for (int frame = 0; frame < 3; ++frame)
	{
		Input::Update();
		expected.ProcessKeyboard(CameraMovement::FORWARD, dt.GetSeconds());
		camera_script.Tick(dt);
		player_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		EXPECT_NEAR(glm::distance(entity.GetComponent<TransformComponent>().translation, expected.GetPosition()), 0.f, 0.00001f);
		EXPECT_TRUE(entity.HasComponent<RenderableObjectComponent>());
	}
	m_input.buttons[DL_GAMEPAD_BUTTON_B] = false;
	Input::Update();
	camera_script.Tick(dt);
	player_script.Tick(dt);
	EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
	EXPECT_FALSE(entity.HasComponent<RenderableObjectComponent>());
	EXPECT_TRUE(entity.HasComponent<SpotLightComponent>());
	EXPECT_EQ(KeybindConfig::FindActionBind("MoveForward")->gamepad_button, DL_GAMEPAD_BUTTON_B);
	EXPECT_EQ(KeybindConfig::FindActionBind("ChangePOV")->gamepad_button, DL_GAMEPAD_BUTTON_B);
}

TEST_F(InputActionsTest, camera_moves_on_first_keyboard_frame_and_ignores_unmapped_sticks)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
	for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
		ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& camera = AddLinkedCamera(entity);
	TestCameraController script(entity);
	const DeltaTime dt(std::chrono::milliseconds(100));
	PerspectiveCamera expected;
	m_input.keys[DL_KEY_W] = true;
	for (int frame = 0; frame < 2; ++frame)
	{
		Input::Update();
		expected.ProcessKeyboard(CameraMovement::FORWARD, dt.GetSeconds());
		script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
	}
	m_input.keys[DL_KEY_W] = false;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 1.f;
	m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = -1.f;
	m_input.axes[DL_GAMEPAD_AXIS_RIGHT_X] = 1.f;
	Input::Update();
	script.Tick(dt);
	EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
	EXPECT_NEAR(camera.GetYaw(), expected.GetYaw(), 0.0001f);
}

TEST_F(InputActionsTest, ExamplePlayerGamepadTogglesOnlyOnReleaseAndRestoresComponents)
{
	ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
	ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
	Input::Update();
	Scene scene;
	auto entity = scene.CreateEntity();
	(void)AddLinkedCamera(entity);
	entity.AddComponent<RenderableObjectComponent>();
	entity.AddComponent<SpotLightComponent>();
	TestPlayerController script(entity);
	for (const bool initially_present : { true, false })
	{
		m_input.buttons[DL_GAMEPAD_BUTTON_Y] = true;
		m_input.buttons[DL_GAMEPAD_BUTTON_B] = true;
		Input::Update();
		script.Tick({});
		EXPECT_EQ(entity.HasComponent<SpotLightComponent>(), initially_present);
		EXPECT_EQ(entity.HasComponent<RenderableObjectComponent>(), initially_present);
		Input::Update();
		script.Tick({});
		EXPECT_EQ(entity.HasComponent<SpotLightComponent>(), initially_present);
		EXPECT_EQ(entity.HasComponent<RenderableObjectComponent>(), initially_present);
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

TEST_F(InputActionsTest, player_waits_for_both_devices_to_release_and_uses_named_axis_for_facing)
{
	ASSERT_EXIT(([&]()
	{
		ASSERT_TRUE(KeybindConfig::SetActionBind("Flashlight", { DL_KEY_F, DL_GAMEPAD_BUTTON_Y }));
		ASSERT_TRUE(KeybindConfig::SetActionBind("ChangePOV", { DL_KEY_V, DL_GAMEPAD_BUTTON_B }));
		for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveForward", { "", "", GamepadAxis::LeftY, -1.f, 0.f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		(void)AddLinkedCamera(entity);
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
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(InputActionsTest, camera_uses_signed_action_named_movement_and_look_axes)
{
	ASSERT_EXIT(([&]()
	{
		for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveForward", { "", "", GamepadAxis::LeftY, -1.f, 0.f }));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveRight", { "", "", GamepadAxis::LeftX, 1.f, 0.f }));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("LookRight", { "", "", GamepadAxis::RightX, 1.f, 0.f }));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("LookUp", { "", "", GamepadAxis::RightY, 1.f, 0.f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = AddLinkedCamera(entity);
		TestCameraController script(entity);
		const DeltaTime dt(std::chrono::milliseconds(100));
		const std::array<std::pair<float, float>, 7> positions{
			std::pair{ 0.f, -0.5f }, { 0.f, 0.5f }, { -0.5f, 0.f }, { 0.5f, 0.f },
			{ 0.3f, -0.7f }, { -0.3f, 0.7f }, { 0.1f, -0.1f } };
		for (const auto& [x, y] : positions)
		{
			SCOPED_TRACE(::testing::Message() << "x=" << x << ", y=" << y);
			ResetCameraPosition(entity);
			m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = x;
			m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = y;
			Input::Update();
			PerspectiveCamera expected;
			expected.ProcessKeyboard(CameraMovement::FORWARD, -y * dt.GetSeconds());
			expected.ProcessKeyboard(CameraMovement::RIGHT, x * dt.GetSeconds());
			script.Tick(dt);
			EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		}
		ResetCameraPosition(entity);
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.f;
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = 0.f;
		m_input.axes[DL_GAMEPAD_AXIS_RIGHT_X] = 0.5f;
		m_input.axes[DL_GAMEPAD_AXIS_RIGHT_Y] = -0.25f;
		PerspectiveCamera expected;
		expected.ProcessMouseMovement(0.5f * dt.GetSeconds() * 100.f, 0.25f * dt.GetSeconds() * 100.f);
		Input::Update();
		script.Tick(dt);
		EXPECT_NEAR(camera.GetYaw(), expected.GetYaw(), 0.0001f);
		EXPECT_NEAR(camera.GetPitch(), expected.GetPitch(), 0.0001f);
		Input::EnableGamepadInput(false);
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 1.f;
		script.Tick(dt);
		EXPECT_NEAR(glm::length(camera.GetPosition()), 0.f, 0.00001f);
		EXPECT_NEAR(camera.GetYaw(), expected.GetYaw(), 0.0001f);
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(InputActionsTest, player_faces_signed_named_axes_and_ignores_neutral_or_disconnected_input)
{
	ASSERT_EXIT(([&]()
	{
		for (const auto* name : { "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "Flashlight", "ChangePOV" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveForward", { "", "", GamepadAxis::LeftY, -1.f, 0.f }));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveRight", { "", "", GamepadAxis::LeftX, 1.f, 0.f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = AddLinkedCamera(entity);
		entity.AddComponent<SpotLightComponent>();
		TestPlayerController script(entity);
		auto& transform = entity.GetComponent<TransformComponent>();
		const std::array<std::pair<float, float>, 6> positions{
			std::pair{ 0.f, -0.5f }, { 0.f, 0.5f }, { -0.5f, 0.f }, { 0.5f, 0.f },
			{ 0.3f, -0.7f }, { -0.3f, 0.7f } };
		for (const float pitch : { 0.f, 60.f, -60.f })
		{
			camera.SetRotation(-45.f, pitch);
			entity.GetLinkedEntity("camera").GetComponent<TransformComponent>().SetOrientationQuat(camera.GetOrientation());
			for (const auto& [x, y] : positions)
			{
				transform.rotation.y = 0.f;
				m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = x;
				m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = y;
				Input::Update();
				script.Tick(DeltaTime(std::chrono::seconds(1)));
				const auto direction = camera.GetDirection() * -y + camera.GetRight() * x;
				float expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
				if (expected_yaw < 0.f) expected_yaw += 360.f;
				EXPECT_NEAR(transform.rotation.y, expected_yaw, 0.0001f);
			}
		}
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveForward", { "", "", GamepadAxis::LeftY, -1.f, 0.2f }));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveRight", { "", "", GamepadAxis::LeftX, 1.f, 0.2f }));
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
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(InputActionsTest, player_combines_immediate_keyboard_input_with_named_analog_axis)
{
	ASSERT_EXIT(([&]()
	{
		ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
		for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "Flashlight", "ChangePOV" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveRight", { "", "", GamepadAxis::LeftX, 1.f, 0.f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = AddLinkedCamera(entity);
		entity.AddComponent<SpotLightComponent>();
		TestPlayerController script(entity);
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.rotation.y = 17.f;
		m_input.keys[DL_KEY_W] = true;
		Input::Update();
		script.Tick(DeltaTime(std::chrono::seconds(1)));
		auto direction = camera.GetDirection();
		float expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
		if (expected_yaw < 0.f) expected_yaw += 360.f;
		EXPECT_NEAR(transform.rotation.y, expected_yaw, 0.0001f);
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.5f;
		Input::Update();
		script.Tick(DeltaTime(std::chrono::seconds(1)));
		direction = camera.GetDirection() + camera.GetRight() * 0.5f;
		expected_yaw = glm::degrees(std::atan2(direction.x, direction.z));
		if (expected_yaw < 0.f) expected_yaw += 360.f;
		EXPECT_NEAR(transform.rotation.y, expected_yaw, 0.0001f);
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(InputActionsTest, action_named_movement_axes_respect_remapping_and_device_gates)
{
	// Config has no axis-removal API; isolate registrations from other tests.
	ASSERT_EXIT(([&]()
	{
		ASSERT_TRUE(KeybindConfig::SetActionBind("MoveForward", { DL_KEY_W, -1 }));
		for (const auto* name : { "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown", "Flashlight", "ChangePOV" })
			ASSERT_TRUE(KeybindConfig::SetActionBind(name, {}));
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveForward", { "", "", GamepadAxis::LeftY, -1.f, 0.2f }));
		// Legacy names are not consumed by the development controllers.
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveHorizontal", { "", "", GamepadAxis::LeftX, 1.f, 0.f }));
		Input::Update();
		Scene scene;
		auto entity = scene.CreateEntity();
		auto& camera = AddLinkedCamera(entity);
		entity.AddComponent<SpotLightComponent>();
		TestCameraController camera_script(entity);
		TestPlayerController player_script(entity);
		const DeltaTime dt(std::chrono::milliseconds(100));
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_X] = 0.4f;
		m_input.axes[DL_GAMEPAD_AXIS_LEFT_Y] = -0.6f;
		Input::Update();
		PerspectiveCamera expected;
		expected.ProcessKeyboard(CameraMovement::FORWARD, 0.5f * dt.GetSeconds());
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		ASSERT_TRUE(KeybindConfig::SetAxisBind("MoveRight", { "", "", GamepadAxis::RightX, -1.f, 0.f }));
		m_input.axes[DL_GAMEPAD_AXIS_RIGHT_X] = 0.25f;
		ResetCameraPosition(entity);
		expected.SetPosition(glm::vec3(0.f));
		expected.ProcessKeyboard(CameraMovement::FORWARD, 0.5f * dt.GetSeconds());
		expected.ProcessKeyboard(CameraMovement::RIGHT, -0.25f * dt.GetSeconds());
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
		ResetCameraPosition(entity);
		expected.SetPosition(glm::vec3(0.f));
		expected.SetRotation(camera.GetYaw(), camera.GetPitch());
		expected.ProcessKeyboard(CameraMovement::FORWARD, dt.GetSeconds());
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		Input::EnableKeyboardInput(false);
		camera_script.Tick(dt);
		EXPECT_NEAR(glm::distance(camera.GetPosition(), expected.GetPosition()), 0.f, 0.00001f);
		std::exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
	}()), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}
