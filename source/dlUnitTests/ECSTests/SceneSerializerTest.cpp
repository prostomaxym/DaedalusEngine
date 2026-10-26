#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/ScriptRegistry.h>
#include <ECS/Entity.h>
#include <ECS/NativeScript.h>

#include <gtest/gtest.h>
#include <fstream>
#include <type_traits>

using namespace Daedalus;

namespace
{
	SceneRecord BasicScene()
	{
		SceneRecord record;
		record.name = "Loaded scene";
		record.primary_camera = "42";
		EntityRecord camera;
		camera.id = "42";
		camera.name = "Not the hard-coded camera name";
		camera.camera.emplace();
		camera.camera->yaw = -90.f;
		camera.camera->distance = 7.f;
		camera.transform.translation = { 3.f, 4.f, 5.f };
		record.entities.push_back(std::move(camera));
		return record;
	}

	class ExampleScript : public NativeScript
	{
	public:
		ExampleScript(Entity entity, double speed) : NativeScript(entity), m_speed(speed) {}
		double GetSpeed() const { return m_speed; }
	private:
		double m_speed;
	};

	bool RegisterExample(ScriptRegistry& registry, bool fail = false)
	{
		return registry.Register<ExampleScript>("ExampleScript",
			[](const ScriptRecord& script, const EntityRecord& entity, std::string& error)
			{
				const auto it = script.properties.find("speed");
				if (!entity.camera || script.properties.size() != 1 || it == script.properties.end() || !std::holds_alternative<double>(it->second))
				{
					error = "requires a camera and numeric speed";
					return false;
				}
				return true;
			},
			[fail](Entity entity, const ScriptRecord& script) -> ExampleScript*
			{
				if (fail) return nullptr;
				return entity.GetComponent<NativeScriptComponent>().AddScript<ExampleScript>(entity, std::get<double>(script.properties.at("speed")));
			},
			[](const ExampleScript& script, ScriptRecord& record) { record.properties["speed"] = script.GetSpeed(); });
	}
}

TEST(SceneSerializerTest, creates_scene_without_runtime_or_graphics)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(BasicScene(), scene, ".", scripts, error)) << error;
	EXPECT_EQ(scene.GetSceneName(), "Loaded scene");
	EXPECT_FALSE(scene.IsRunning());
	ASSERT_NE(scene.GetCamera(), nullptr);
	EXPECT_FLOAT_EQ(scene.GetCamera()->GetYaw(), -90.f);
	EXPECT_FLOAT_EQ(scene.GetCamera()->GetPersonDistance(), 7.f);
	EXPECT_TRUE(scene.GetEntityByUUID(Daedalus::UUID(42)));
}

TEST(SceneSerializerTest, capture_instantiation_round_trip_preserves_explicit_camera)
{
	Scene first, second;
	ScriptRegistry scripts;
	std::string error, a, b;
	auto input = BasicScene();
	input.entities[0].directional_light.emplace();
	input.entities[0].directional_light->max_distance = 123.f;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, first, ".", scripts, error)) << error;
	SceneRecord captured, recaptured;
	ASSERT_TRUE(SceneSerializer::Capture(first, ".", scripts, captured, error)) << error;
	ASSERT_TRUE(SceneSerializer::Instantiate(captured, second, ".", scripts, error)) << error;
	ASSERT_TRUE(SceneSerializer::Capture(second, ".", scripts, recaptured, error)) << error;
	ASSERT_TRUE(SceneJson::Serialize(captured, a, error)) << error;
	ASSERT_TRUE(SceneJson::Serialize(recaptured, b, error)) << error;
	EXPECT_EQ(a, b);
	EXPECT_EQ(captured.primary_camera, "42");
	EXPECT_FLOAT_EQ(captured.entities[0].directional_light->max_distance, 123.f);
}

TEST(SceneSerializerTest, refuses_to_overwrite_existing_entities)
{
	Scene scene;
	scene.CreateEntityWithUUID(Daedalus::UUID(7), "Existing");
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(BasicScene(), scene, ".", scripts, error));
	EXPECT_TRUE(scene.GetEntityByUUID(Daedalus::UUID(7)));
	EXPECT_FALSE(scene.GetEntityByUUID(Daedalus::UUID(42)));
}

TEST(SceneSerializerTest, invalid_record_leaves_scene_metadata_unchanged)
{
	Scene scene;
	scene.SetSceneName("Original");
	scene.SetPaused(true);
	auto input = BasicScene();
	input.primary_camera = "99";
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, ".", scripts, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetSceneName(), "Original");
	EXPECT_TRUE(scene.IsPaused());
}

TEST(SceneSerializerTest, missing_asset_is_rejected_before_entities_are_created)
{
	auto input = BasicScene();
	input.entities[0].model = ModelRecord{ "nonexistent_serialization_test_asset.obj", 0 };
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, DL_SCENE_FIXTURES, scripts, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_NE(error.find("Missing asset"), std::string::npos);
}

TEST(SceneSerializerTest, unknown_script_is_rejected_before_instantiation)
{
	auto input = BasicScene();
	input.entities[0].scripts.push_back({ "Unknown", {} });
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, ".", scripts, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_NE(error.find("unregistered"), std::string::npos);
}

TEST(SceneSerializerTest, script_properties_are_validated_before_construction)
{
	auto input = BasicScene();
	input.entities[0].scripts.push_back({ "ExampleScript", { { "speed", std::string("wrong type") } } });
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(RegisterExample(scripts));
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, ".", scripts, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_NE(error.find("numeric speed"), std::string::npos);
}

TEST(SceneSerializerTest, script_parameters_survive_capture)
{
	auto input = BasicScene();
	input.entities[0].scripts.push_back({ "ExampleScript", { { "speed", 12.5 } } });
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(RegisterExample(scripts));
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
	const auto* script = scene.GetEntityByUUID(Daedalus::UUID(42)).GetComponent<NativeScriptComponent>().GetScript<ExampleScript>();
	ASSERT_NE(script, nullptr);
	EXPECT_DOUBLE_EQ(script->GetSpeed(), 12.5);
	SceneRecord output;
	ASSERT_TRUE(SceneSerializer::Capture(scene, ".", scripts, output, error)) << error;
	EXPECT_DOUBLE_EQ(std::get<double>(output.entities[0].scripts[0].properties.at("speed")), 12.5);
}

TEST(SceneSerializerTest, failed_script_factory_rolls_back_scene_and_allows_retry)
{
	auto input = BasicScene();
	input.entities[0].point_light.emplace();
	input.entities[0].scripts.push_back({ "ExampleScript", { { "speed", 12.5 } } });
	Scene scene;
	scene.SetSceneName("Original");
	scene.SetPaused(true);
	ScriptRegistry broken, working;
	ASSERT_TRUE(RegisterExample(broken, true));
	ASSERT_TRUE(RegisterExample(working));
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, ".", broken, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetSceneName(), "Original");
	EXPECT_TRUE(scene.IsPaused());
	EXPECT_EQ(scene.GetCamera(), nullptr);
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", working, error)) << error;
}

TEST(SceneSerializerTest, rejects_duplicate_script_registration)
{
	ScriptRegistry scripts;
	EXPECT_TRUE(RegisterExample(scripts));
	EXPECT_FALSE(RegisterExample(scripts));
}

TEST(SceneSerializerTest, clear_removes_entities_and_primary_camera)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(BasicScene(), scene, ".", scripts, error)) << error;
	scene.Clear();
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetCamera(), nullptr);
	EXPECT_FALSE(scene.GetEntityByUUID(Daedalus::UUID(42)));
}

TEST(SceneSerializerTest, duplicate_uuid_does_not_corrupt_lookup)
{
	Scene scene;
	const auto first = scene.CreateEntityWithUUID(Daedalus::UUID(42), "First");
	EXPECT_FALSE(scene.CreateEntityWithUUID(Daedalus::UUID(42), "Duplicate"));
	EXPECT_EQ(scene.GetEntityByUUID(Daedalus::UUID(42)), first);
}

TEST(SceneSerializerTest, viewport_policy_updates_only_opted_in_perspective_cameras)
{
	auto input = BasicScene();
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
	scene.OnViewportResize(1000, 500);
	const auto* camera = dynamic_cast<PerspectiveCamera*>(scene.GetCamera());
	ASSERT_NE(camera, nullptr);
	EXPECT_FLOAT_EQ(camera->GetProjectionProps().aspect_ratio, 2.f);
	scene.GetEntityByUUID(Daedalus::UUID(42)).GetComponent<CameraComponent>().viewport_aspect = false;
	scene.OnViewportResize(500, 500);
	EXPECT_FLOAT_EQ(camera->GetProjectionProps().aspect_ratio, 2.f);
}

TEST(SceneSerializerTest, orthographic_projection_survives_capture)
{
	auto input = BasicScene();
	auto& camera = *input.entities[0].camera;
	camera.projection = "orthographic";
	camera.left = -20.f; camera.right = 20.f; camera.bottom = -10.f; camera.top = 10.f;
	camera.near_plane = -100.f; camera.far_plane = 100.f;
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
	SceneRecord output;
	ASSERT_TRUE(SceneSerializer::Capture(scene, ".", scripts, output, error)) << error;
	EXPECT_EQ(output.entities[0].camera->projection, "orthographic");
	EXPECT_FLOAT_EQ(output.entities[0].camera->left, -20.f);
	EXPECT_FLOAT_EQ(output.entities[0].camera->near_plane, -100.f);
}

TEST(SceneSerializerTest, light_setter_changes_survive_capture)
{
	auto input = BasicScene();
	input.entities[0].spot_light.emplace();
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
	auto& light = scene.GetEntityByUUID(Daedalus::UUID(42)).GetComponent<SpotLightComponent>().light;
	light.SetCutoffAngle(glm::cos(glm::radians(20.f)));
	light.SetPosition({ 1.f, 2.f, 3.f });
	light.SetMaxDistance(123.f);
	light.SetPower(5.f);
	SceneRecord output;
	ASSERT_TRUE(SceneSerializer::Capture(scene, ".", scripts, output, error)) << error;
	const auto& captured = *output.entities[0].spot_light;
	EXPECT_NEAR(captured.inner_angle, 20.f, 0.0001f);
	EXPECT_FLOAT_EQ(captured.position.y, 2.f);
	EXPECT_FLOAT_EQ(captured.max_distance, 123.f);
	EXPECT_FLOAT_EQ(captured.power, 5.f);
}

TEST(SceneSerializerTest, scene_metadata_and_all_entities_exist_before_script_construction)
{
	auto input = BasicScene();
	input.entities[0].scripts.push_back({ "ExampleScript", {} });
	EntityRecord other;
	other.id = "99";
	input.entities.push_back(other);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(scripts.Register<ExampleScript>("ExampleScript",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[&scene](Entity entity, const ScriptRecord&) -> ExampleScript*
		{
			if (!scene.GetCamera() || scene.GetSceneName() != "Loaded scene" || !scene.GetEntityByUUID(Daedalus::UUID(99))) return nullptr;
			return entity.GetComponent<NativeScriptComponent>().AddScript<ExampleScript>(entity, 1.0);
		},
		[](const ExampleScript&, ScriptRecord&) {}));
	std::string error;
	EXPECT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
}

class SceneBodyTest : public ::testing::Test
{
};

TEST_F(SceneBodyTest, body_pose_remains_independent_of_entity_transform)
{
	auto input = BasicScene();
	input.entities[0].rigid_body.emplace();
	input.entities[0].rigid_body->position = { 10.f, 20.f, 30.f };
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(input, scene, ".", scripts, error)) << error;
	const auto handle = scene.GetEntityByUUID(Daedalus::UUID(42)).GetComponent<RigidBodyComponent>().handle;
	glm::vec3 position;
	glm::quat orientation;
	ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(handle, position, orientation));
	EXPECT_FLOAT_EQ(position.x, 10.f);
	EXPECT_FLOAT_EQ(position.y, 20.f);
	SceneRecord output;
	ASSERT_TRUE(SceneSerializer::Capture(scene, ".", scripts, output, error)) << error;
	EXPECT_FLOAT_EQ(output.entities[0].rigid_body->position.z, 30.f);
	EXPECT_FLOAT_EQ(output.entities[0].transform.translation.x, 3.f);
}

TEST_F(SceneBodyTest, moving_component_transfers_body_ownership)
{
	static_assert(!std::is_copy_constructible_v<RigidBodyComponent>);
	static_assert(!std::is_copy_constructible_v<KinematicBodyComponent>);
	RigidBodyComponent original(1.f, 0.5f, glm::vec3(1.f));
	PhysicsWorld world;
	original.Attach(world);
	const auto handle = original.handle;
	RigidBodyComponent moved(std::move(original));
	EXPECT_EQ(original.handle, 0u);
	EXPECT_EQ(moved.handle, handle);
	RigidBodyComponent destination(2.f, 1.f, glm::vec3(2.f));
	destination.Attach(world);
	const auto replaced = destination.handle;
	destination = std::move(moved);
	EXPECT_EQ(moved.handle, 0u);
	EXPECT_EQ(destination.handle, handle);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_TRUE(world.GetRigidBodyTransform(destination.handle, position, orientation));
	EXPECT_FALSE(world.GetRigidBodyTransform(replaced, position, orientation));
}

TEST_F(SceneBodyTest, simulation_and_cleanup_are_scene_local)
{
	Scene first, second;
	auto a = first.CreateEntity();
	auto b = second.CreateEntity();
	const auto first_handle = a.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f)).handle;
	const auto second_handle = b.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f)).handle;
	first.GetPhysicsWorld().StepSimulation();
	glm::vec3 position;
	glm::quat orientation;
	ASSERT_TRUE(first.GetPhysicsWorld().GetRigidBodyTransform(first_handle, position, orientation));
	EXPECT_LT(position.y, 10.f);
	ASSERT_TRUE(second.GetPhysicsWorld().GetRigidBodyTransform(second_handle, position, orientation));
	EXPECT_FLOAT_EQ(position.y, 10.f);
	first.Clear();
	EXPECT_FALSE(first.GetPhysicsWorld().GetRigidBodyTransform(first_handle, position, orientation));
	EXPECT_TRUE(second.GetPhysicsWorld().GetRigidBodyTransform(second_handle, position, orientation));
}

TEST_F(SceneBodyTest, component_removal_releases_its_body)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	const auto handle = entity.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f)).handle;
	entity.RemoveComponent<RigidBodyComponent>();
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_FALSE(scene.GetPhysicsWorld().GetRigidBodyTransform(handle, position, orientation));
}

TEST_F(SceneBodyTest, detached_component_can_outlive_its_scene)
{
	std::unique_ptr<RigidBodyComponent> detached;
	{
		Scene scene;
		auto entity = scene.CreateEntity();
		entity.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f));
		detached = entity.PopComponent<RigidBodyComponent>();
		EXPECT_NE(detached->handle, 0u);
	}
	detached.reset();
}

TEST_F(SceneBodyTest, pushing_moved_component_binds_the_stored_instance)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	RigidBodyComponent component(1.f, 0.5f, glm::vec3(1.f));
	entity.PushComponent(std::move(component));
	EXPECT_EQ(component.handle, 0u);
	const auto handle = entity.GetComponent<RigidBodyComponent>().handle;
	EXPECT_NE(handle, 0u);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(handle, position, orientation));
}

TEST_F(SceneBodyTest, pushing_detached_component_rebinds_to_destination_world)
{
	Scene first, second;
	auto source = first.CreateEntity();
	const auto old_handle = source.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f)).handle;
	auto component = source.PopComponent<RigidBodyComponent>();
	auto destination = second.CreateEntity();
	destination.PushComponent(std::move(component));
	EXPECT_EQ(component, nullptr);
	const auto new_handle = destination.GetComponent<RigidBodyComponent>().handle;
	EXPECT_NE(new_handle, 0u);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_FALSE(first.GetPhysicsWorld().GetRigidBodyTransform(old_handle, position, orientation));
	EXPECT_TRUE(second.GetPhysicsWorld().GetRigidBodyTransform(new_handle, position, orientation));
}

class SceneFileTest : public ::testing::Test
{
protected:
	std::filesystem::path m_directory;
	void SetUp() override
	{
		m_directory = std::filesystem::path(DL_SCENE_TEST_OUTPUT) / ::testing::UnitTest::GetInstance()->current_test_info()->name();
		std::error_code ec;
		std::filesystem::create_directories(m_directory, ec);
		ASSERT_FALSE(ec) << ec.message();
	}
	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_directory, ec);
	}
};

TEST_F(SceneFileTest, save_replaces_existing_file_without_leaving_recovery_files)
{
	const auto path = m_directory / "scene.json";
	auto input = BasicScene();
	std::string error;
	ASSERT_TRUE(SceneJson::Save(path, input, error)) << error;
	input.name = "Replaced";
	ASSERT_TRUE(SceneJson::Save(path, input, error)) << error;
	SceneRecord output;
	ASSERT_TRUE(SceneJson::Load(path, output, error)) << error;
	EXPECT_EQ(output.name, "Replaced");
	EXPECT_FALSE(std::filesystem::exists(m_directory / "scene.json.tmp"));
	EXPECT_FALSE(std::filesystem::exists(m_directory / "scene.json.bak"));
}

TEST_F(SceneFileTest, validation_failure_does_not_overwrite_existing_file)
{
	const auto path = m_directory / "scene.json";
	auto input = BasicScene();
	std::string error;
	ASSERT_TRUE(SceneJson::Save(path, input, error)) << error;
	input.version = 2;
	EXPECT_FALSE(SceneJson::Save(path, input, error));
	SceneRecord output;
	ASSERT_TRUE(SceneJson::Load(path, output, error)) << error;
	EXPECT_EQ(output.version, 1u);
}

TEST_F(SceneFileTest, pending_recovery_file_is_not_overwritten)
{
	const auto path = m_directory / "scene.json";
	std::ofstream(m_directory / "scene.json.bak") << "preserved";
	std::string error;
	EXPECT_FALSE(SceneJson::Save(path, BasicScene(), error));
	EXPECT_TRUE(std::filesystem::exists(m_directory / "scene.json.bak"));
	EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(SceneFileTest, file_api_round_trip_loads_entities_not_just_records)
{
	const auto path = m_directory / "scene.json";
	Scene first, second;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(BasicScene(), first, m_directory, scripts, error)) << error;
	ASSERT_TRUE(SceneSerializer::Save(path, first, m_directory, scripts, error)) << error;
	ASSERT_TRUE(SceneSerializer::Load(path, second, m_directory, scripts, error)) << error;
	EXPECT_TRUE(second.GetEntityByUUID(Daedalus::UUID(42)));
	EXPECT_EQ(second.GetSceneName(), "Loaded scene");
}

TEST_F(SceneFileTest, save_does_not_rename_or_replace_a_directory)
{
	const auto path = m_directory / "scene.json";
	std::filesystem::create_directory(path);
	std::ofstream(path / "preserved.txt") << "original";
	std::string error;
	EXPECT_FALSE(SceneJson::Save(path, BasicScene(), error));
	EXPECT_TRUE(std::filesystem::is_directory(path));
	EXPECT_TRUE(std::filesystem::exists(path / "preserved.txt"));
	EXPECT_FALSE(std::filesystem::exists(m_directory / "scene.json.bak"));
}

TEST_F(SceneFileTest, failed_model_import_rolls_back_previously_created_entities)
{
	std::ofstream(m_directory / "invalid.obj") << "not a model";
	auto input = BasicScene();
	input.entities[0].point_light.emplace();
	EntityRecord model;
	model.id = "99";
	model.model = ModelRecord{ "invalid.obj", 0 };
	input.entities.push_back(model);
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(input, scene, m_directory, scripts, error));
	EXPECT_NE(error.find("Failed to load model"), std::string::npos);
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetCamera(), nullptr);
}
