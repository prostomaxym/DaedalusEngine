#include <ECS/Scripting/LuaScript.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/ScriptRegistry.h>
#include <Core/Input.h>
#include <Core/AssetPipeline.h>
#include <Config/PathConfig.h>
#include <Events/GameEvent.h>
#include <Events/KeyEvent.h>
#include <Events/MouseEvent.h>
#include <Events/GamepadEvent.h>
#include <Events/EventsEngine.h>
#include <Events/PhysicsEvent.h>
#include <gtest/gtest.h>
#include <zip.h>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <thread>

using namespace Daedalus;

namespace
{
	struct PathDefaults
	{
		~PathDefaults() { PathConfig::LoadDefault(); }
	};

	class LuaRuntimeTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			m_root = std::filesystem::path(DL_SCENE_TEST_OUTPUT) / "lua" / ::testing::UnitTest::GetInstance()->current_test_info()->name();
			std::filesystem::remove_all(m_root);
			std::filesystem::create_directories(m_root);
			std::string error;
			ASSERT_TRUE(m_files.Mount("lua", m_root, error)) << error;
			m_entity = m_scene.CreateEntityWithUUID(Daedalus::UUID(9007199254740993ull), "Owner");
		}

		void TearDown() override
		{
			m_scene.Clear();
			std::filesystem::remove_all(m_root);
		}

		void Write(const std::string& source, const std::string& path = "script.lua")
		{
			std::ofstream output(m_root / path, std::ios::binary | std::ios::trunc);
			output.write(source.data(), static_cast<std::streamsize>(source.size()));
			ASSERT_TRUE(output.good());
		}

		void Load(const std::string& source, LuaRuntime::Properties properties = {})
		{
			Write(source);
			m_runtime = std::make_unique<LuaRuntime>(m_scene, m_entity.GetUUID(), m_files, "lua:/script.lua", std::move(properties));
			ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
		}

		void LoadExample(const std::string& name, LuaRuntime::Properties properties = {})
		{
			ASSERT_TRUE(m_files.Mount("scripts", std::filesystem::path(DL_SANDBOX_EXAMPLES) / "scripts"));
			m_runtime = std::make_unique<LuaRuntime>(m_scene, m_entity.GetUUID(), m_files, "scripts:/" + name, std::move(properties));
			ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
		}

		static double Number(const LuaRuntime& runtime, const std::string& key)
		{
			const auto state = runtime.GetState();
			const auto found = state.find(key);
			if (found == state.end() || !std::holds_alternative<double>(found->second))
			{
				ADD_FAILURE() << "Missing numeric state: " << key;
				return 0.;
			}
			return std::get<double>(found->second);
		}

		ScriptRecord Record() const
		{
			return { "Lua", { { "asset", std::string("lua:/script.lua") }, { "speed", 2.5 } } };
		}

		SceneRecord Definition() const
		{
			SceneRecord scene;
			scene.name = "Lua scene";
			EntityRecord entity;
			entity.id = "42";
			entity.name = "Scripted";
			entity.scripts.push_back(Record());
			scene.entities.push_back(entity);
			return scene;
		}

		LuaScript* Attach(Entity entity, const std::string& source)
		{
			Write(source);
			if (!entity.HasComponent<NativeScriptComponent>()) entity.AddComponent<NativeScriptComponent>();
			return entity.GetComponent<NativeScriptComponent>().AddScript<LuaScript>(entity, m_files, Record());
		}

		std::filesystem::path m_root;
		Scene m_scene;
		Entity m_entity;
		VirtualFileSystem m_files;
		std::unique_ptr<LuaRuntime> m_runtime;
	};

	class LuaInput : public Input
	{
	public:
		bool down{ false };
	private:
		void UpdateImpl() override {}
		bool IsKeyPressedImpl(int code) override { return code == DL_KEY_SPACE && down; }
		bool IsKeyHoldImpl(int) override { return false; }
		bool IsKeyReleasedImpl(int) override { return false; }
		bool IsMouseButtonPressedImpl(int) override { return false; }
		std::pair<float, float> GetMousePosImpl() override { return {}; }
		bool IsGpadKeyPressedImpl(int) override { return false; }
		std::pair<float, float> GetStickPosImpl(StickType) override { return {}; }
		float GetBumperPosImpl(BamperType) override { return -1.f; }
	};
}

TEST_F(LuaRuntimeTest, updates_local_transform_from_authored_properties)
{
	Load("function on_update(dt) local p=ecs.position(ecs.self); p.x=p.x+properties.speed*dt; assert(ecs.set_position(ecs.self,p)) end", { { "speed", 4. } });
	m_runtime->Update(0.25f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 1.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
}

TEST_F(LuaRuntimeTest, sandbox_player_follows_linked_camera_and_restores_presentation_on_stop)
{
	auto parent = m_scene.CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().rotation.y = 30.f;
	ASSERT_TRUE(m_scene.SetParent(m_entity.GetUUID(), parent.GetUUID()));
	m_entity.GetComponent<TransformComponent>().rotation.y = 180.f;
	m_entity.AddComponent<RenderableObjectComponent>();
	auto camera = m_scene.CreateEntity("Camera");
	camera.GetComponent<TransformComponent>().translation = { 2.f, 3.f, 4.f };
	camera.AddComponent<CameraComponent>(CameraProjectionProps{});
	auto light = m_scene.CreateEntity("Flashlight");
	LightProps props;
	props.light_power = 4.f;
	light.AddComponent<SpotLightComponent>(props);
	ASSERT_TRUE(m_scene.SetEntityLink(m_entity.GetUUID(), "camera", camera.GetUUID()));
	ASSERT_TRUE(m_scene.SetEntityLink(m_entity.GetUUID(), "spot_light", light.GetUUID()));
	LoadExample("player_controller.lua");
	m_runtime->Update(0.f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	m_scene.UpdateWorldTransforms();
	EXPECT_NEAR(glm::distance(glm::vec3((*m_scene.GetWorldTransform(m_entity.GetUUID()))[3]), glm::vec3(2.f, 3.f, 4.f)), 0.f, 0.0001f);
	EXPECT_NEAR(glm::distance(glm::vec3((*m_scene.GetWorldTransform(light.GetUUID()))[3]), glm::vec3(2.f, 3.f, 3.f)), 0.f, 0.0001f);
	const auto expected = m_scene.GetWorldOrientation(m_entity.GetUUID()) * glm::angleAxis(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
	EXPECT_NEAR(std::abs(glm::dot(expected, m_scene.GetWorldOrientation(light.GetUUID()))), 1.f, 0.0001f);
	m_entity.GetComponent<RenderableObjectComponent>().visible = false;
	light.GetComponent<SpotLightComponent>().light.SetPower(0.f);
	m_runtime->Stop();
	EXPECT_TRUE(m_entity.GetComponent<RenderableObjectComponent>().visible);
	EXPECT_FLOAT_EQ(light.GetComponent<SpotLightComponent>().light.GetProps().light_power, 4.f);
	EXPECT_FLOAT_EQ(camera.GetComponent<CameraComponent>().camera->GetPersonDistance(), 7.f);
}

TEST_F(LuaRuntimeTest, configured_script_mount_loads_reloads_and_captures_lua_records)
{
	const PathDefaults restore_paths;
	std::filesystem::create_directories(m_root / "lua source");
	std::filesystem::create_directories(m_root / "other_assets");
	Write("function on_update(dt) state.value=1 end", "lua source/mover.lua");
	Write("not valid Lua source", "other_assets/mover.lua");
	Write("[Path]\nAssetsFolder=other_assets\nScriptsFolder=lua source\n", "path.ini");
	PathConfig::Load(m_root / "path.ini");
	VirtualFileSystem files;
	ASSERT_TRUE(files.Mount("scripts", PathConfig::GetScriptsPath()));
	ASSERT_TRUE(files.Mount("assets", PathConfig::GetAssetsPath()));
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, std::move(files));
	ASSERT_TRUE(pipeline.Initialize());
	ASSERT_NE(pipeline.GetRegistry().Find("scripts:/mover.lua"), nullptr);
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, pipeline.GetFiles()));
	auto record = Definition();
	record.entities[0].scripts[0].properties["asset"] = std::string("scripts:/mover.lua");
	Scene scene;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(record, scene, PathConfig::GetAssetsPath(), registry, error)) << error;
	scene.UpdateScripts({});
	auto* script = scene.GetEntityByUUID(Daedalus::UUID(42)).GetComponent<NativeScriptComponent>().GetScript<LuaScript>();
	ASSERT_NE(script, nullptr);
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "value"), 1.);
	Write("function on_update(dt) state.value=2 end", "lua source/mover.lua");
	scene.UpdateScripts(DeltaTime(std::chrono::milliseconds(500)));
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "value"), 2.);
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, PathConfig::GetAssetsPath(), registry, captured, error)) << error;
	ASSERT_EQ(captured.entities.size(), 1u);
	EXPECT_EQ(std::get<std::string>(captured.entities[0].scripts[0].properties.at("asset")), "scripts:/mover.lua");
}

TEST_F(LuaRuntimeTest, existing_explicit_asset_mount_script_paths_remain_valid)
{
	Write("function on_update(dt) state.value=42 end");
	ASSERT_TRUE(m_files.Mount("assets", m_root));
	LuaRuntime runtime(m_scene, m_entity.GetUUID(), m_files, "assets:/script.lua");
	runtime.Update(0.f);
	EXPECT_TRUE(runtime.IsReady()) << runtime.GetLastError();
	EXPECT_DOUBLE_EQ(Number(runtime, "value"), 42.);
}

TEST_F(LuaRuntimeTest, starts_once_and_restarts_from_authored_state)
{
	Load("function on_start() state.starts=state.starts+1 end; function on_update(dt) state.ticks=(state.ticks or 0)+1 end", { { "starts", 0. } });
	m_runtime->Update(0.1f);
	m_runtime->Update(0.1f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "starts"), 1.);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ticks"), 2.);
	m_runtime->Stop();
	EXPECT_FALSE(m_runtime->IsReady());
	m_runtime->Update(0.1f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "starts"), 1.);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ticks"), 1.);
}

TEST_F(LuaRuntimeTest, empty_sources_initialize_automatically_and_after_restart)
{
	Write("");
	LuaRuntime runtime(m_scene, m_entity.GetUUID(), m_files, "lua:/script.lua");
	runtime.Update(0.f);
	ASSERT_TRUE(runtime.IsReady()) << runtime.GetLastError();
	runtime.Update(0.5f);
	EXPECT_TRUE(runtime.IsReady());
	runtime.Stop();
	runtime.Update(0.f);
	EXPECT_TRUE(runtime.IsReady()) << runtime.GetLastError();
}

TEST_F(LuaRuntimeTest, preserves_large_uuid_strings_without_numeric_rounding)
{
	Load("function on_update(dt) assert(ecs.self=='9007199254740993'); assert(ecs.valid(ecs.self)); assert(ecs.find('Owner')==ecs.self); assert(ecs.name(ecs.self)=='Owner'); state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
}

TEST_F(LuaRuntimeTest, resolves_links_and_rejects_destroyed_entity_handles)
{
	auto target = m_scene.CreateEntityWithUUID(Daedalus::UUID(77), "Target");
	ASSERT_TRUE(m_scene.SetEntityLink(m_entity.GetUUID(), "target", target.GetUUID()));
	Load("function on_update(dt) if not state.target then state.target=ecs.link(ecs.self,'target') end; state.valid=ecs.valid(state.target); if not state.valid then assert(ecs.position(state.target)==nil); assert(not ecs.set_position(state.target,vec3(1,2,3))) end end");
	m_runtime->Update(0.f);
	EXPECT_TRUE(std::get<bool>(m_runtime->GetState().at("valid")));
	m_scene.DestroyEntity(target);
	m_runtime->Update(0.f);
	EXPECT_FALSE(std::get<bool>(m_runtime->GetState().at("valid")));
}

TEST_F(LuaRuntimeTest, transform_bindings_preserve_local_parent_space)
{
	auto parent = m_scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	ASSERT_TRUE(m_scene.SetParent(m_entity.GetUUID(), parent.GetUUID()));
	Load("function on_update(dt) assert(ecs.set_position(ecs.self,vec3(2,3,4))); assert(ecs.set_rotation(ecs.self,vec3(0,90,0))); assert(ecs.set_scale(ecs.self,vec3(2,2,2))); state.yaw=ecs.rotation(ecs.self).y; state.scale=ecs.scale(ecs.self).x end");
	m_runtime->Update(0.f);
	m_scene.UpdateWorldTransforms();
	ASSERT_NE(m_scene.GetWorldTransform(m_entity.GetUUID()), nullptr);
	EXPECT_FLOAT_EQ((*m_scene.GetWorldTransform(m_entity.GetUUID()))[3].x, 12.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "yaw"), 90.);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "scale"), 2.);
}

TEST_F(LuaRuntimeTest, rejects_nonfinite_and_malformed_ecs_access)
{
	Load("function on_update(dt) assert(not ecs.valid('1x')); assert(not ecs.valid('-1')); assert(not ecs.set_position(ecs.self,vec3(0/0,0,0))); assert(not ecs.set_scale(ecs.self,vec3(math.huge,1,1))); state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 0.f);
}

TEST_F(LuaRuntimeTest, headless_input_queries_return_neutral_values)
{
	ASSERT_FALSE(Input::IsAvailable());
	Load("function on_update(dt) assert(not input.down('missing')); assert(not input.pressed('missing')); assert(not input.held('missing')); assert(not input.released('missing')); assert(input.axis('missing')==0); state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
}

TEST_F(LuaRuntimeTest, input_bindings_observe_named_actions_and_live_gates)
{
	LuaInput input;
	ASSERT_TRUE(KeybindConfig::SetActionBind("lua_action", { DL_KEY_SPACE, -1 }));
	Input::Update();
	Load("function on_update(dt) state.down=input.down('lua_action'); state.pressed=input.pressed('lua_action'); state.released=input.released('lua_action') end");
	input.down = true;
	Input::Update();
	m_runtime->Update(0.f);
	EXPECT_TRUE(std::get<bool>(m_runtime->GetState().at("down")));
	EXPECT_TRUE(std::get<bool>(m_runtime->GetState().at("pressed")));
	Input::Update();
	Input::EnableKeyboardInput(false);
	m_runtime->Update(0.f);
	EXPECT_FALSE(std::get<bool>(m_runtime->GetState().at("down")));
	EXPECT_TRUE(std::get<bool>(m_runtime->GetState().at("released")));
}

TEST_F(LuaRuntimeTest, reload_hook_cannot_commit_invalid_callbacks)
{
	Load("function on_update(dt) state.value=1 end");
	m_runtime->Update(0.f);
	Write("function on_reload() on_update=42 end");
	EXPECT_FALSE(m_runtime->Reload());
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "value"), 1.);
}

TEST_F(LuaRuntimeTest, repaired_faulted_scripts_with_invalid_state_restart_from_properties)
{
	Load("function on_update(dt) state.nested={}; error('bad state') end", { { "value", 7. } });
	m_runtime->Update(0.f);
	ASSERT_FALSE(m_runtime->IsReady());
	Write("function on_update(dt) state.value=state.value+1 end");
	ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "value"), 8.);
}

TEST_F(LuaRuntimeTest, wrong_binding_arguments_are_rejected_before_native_conversion)
{
	Load("function on_update(dt) assert(not ecs.set_position(ecs.self,'not a vector')); assert(not ecs.set_position(ecs.self,{})); assert(not ecs.set_position(ecs.self,nil)); assert(not ecs.set_position({},vec3(1,2,3))); assert(not ecs.valid({})); assert(ecs.position(nil)==nil); assert(vec3({},1,2)==nil); assert(not input.down({})); assert(input.axis({})==0); assert(not events.emit({},'')); state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
}

TEST_F(LuaRuntimeTest, sandbox_omits_external_io_dynamic_loading_and_budget_escape_apis)
{
	Load("assert(io==nil and os==nil and debug==nil and package==nil and coroutine==nil); assert(load==nil and loadfile==nil and dofile==nil and require==nil); assert(pcall==nil and xpcall==nil and getmetatable==nil and setmetatable==nil and collectgarbage==nil and string.dump==nil); assert(ecs==nil and events==nil and input==nil); function on_update(dt) state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
}

TEST_F(LuaRuntimeTest, physics_events_are_scene_scoped_and_expose_copied_contacts)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0 end; function on_event(e) assert(e.type=='PhysicsContact' and e.first=='1' and e.second=='2'); assert(e.phase==0 and e.trigger and e.position.x==3 and e.normal.y==1 and e.impulse==4); state.events=state.events+1 end");
	m_scene.UpdateScripts({});
	PhysicsContact contact;
	contact.trigger = true;
	contact.position.x = 3.f;
	contact.normal.y = 1.f;
	contact.impulse = 4.f;
	Scene other;
	EXPECT_FALSE(m_scene.QueueScriptEvent(std::make_unique<ScenePhysicsEvent>(static_cast<uint64_t>(other.GetPhysicsEventSource()), 1, 2, contact)));
	EXPECT_TRUE(m_scene.QueueScriptEvent(std::make_unique<ScenePhysicsEvent>(static_cast<uint64_t>(m_scene.GetPhysicsEventSource()), 1, 2, contact)));
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 1.);
}

TEST_F(LuaRuntimeTest, entity_vms_do_not_share_globals_or_library_mutations)
{
	Load("function on_update(dt) math.sqrt=nil; secret=42; state.ok=1 end");
	m_runtime->Update(0.f);
	Write("function on_update(dt) assert(secret==nil); state.root=math.sqrt(9) end", "other.lua");
	LuaRuntime other(m_scene, m_entity.GetUUID(), m_files, "lua:/other.lua");
	other.Update(0.f);
	EXPECT_DOUBLE_EQ(Number(other, "root"), 3.);
}

TEST_F(LuaRuntimeTest, instruction_budget_stops_infinite_update_loops)
{
	Load("function on_update(dt) while true do end end");
	m_runtime->Update(0.f);
	EXPECT_FALSE(m_runtime->IsReady());
	EXPECT_NE(m_runtime->GetLastError().find("instruction budget"), std::string::npos);
	m_runtime->Update(0.f);
	EXPECT_FALSE(m_runtime->IsReady());
}

TEST_F(LuaRuntimeTest, instruction_budget_also_bounds_top_level_and_reload_callbacks)
{
	Write("while true do end");
	LuaRuntime runtime(m_scene, m_entity.GetUUID(), m_files, "lua:/script.lua");
	EXPECT_FALSE(runtime.Reload());
	EXPECT_NE(runtime.GetLastError().find("instruction budget"), std::string::npos);
	Write("function on_reload() while true do end end");
	ASSERT_TRUE(runtime.Reload());
	runtime.Update(0.f);
	EXPECT_FALSE(runtime.Reload());
	EXPECT_TRUE(runtime.IsReady());
}

TEST_F(LuaRuntimeTest, memory_budget_stops_large_lua_allocations)
{
	Load("function on_update(dt) state.blob=string.rep('x',32*1024*1024) end");
	m_runtime->Update(0.f);
	EXPECT_FALSE(m_runtime->IsReady());
	EXPECT_FALSE(m_runtime->GetLastError().empty());
	Write("function on_update(dt) state.recovered=1 end");
	ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "recovered"), 1.);
}

TEST_F(LuaRuntimeTest, failed_reload_keeps_previous_code_and_state)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	m_runtime->Update(0.f);
	Write("function on_update(");
	EXPECT_FALSE(m_runtime->Reload());
	EXPECT_TRUE(m_runtime->IsReady());
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "count"), 2.);
}

TEST_F(LuaRuntimeTest, reload_is_staged_without_engine_mutation_capabilities)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	m_runtime->Update(0.f);
	Write("ecs.set_position(ecs.self,vec3(9,9,9))");
	EXPECT_FALSE(m_runtime->Reload());
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "count"), 1.);
}

TEST_F(LuaRuntimeTest, successful_reload_preserves_typed_state_without_restarting)
{
	Load("function on_start() state.starts=(state.starts or 0)+1 end; function on_update(dt) state.count=(state.count or 0)+1; state.text='hello'; state.vector=vec3(1,2,3); state.flag=true end");
	m_runtime->Update(0.f);
	Write("function on_start() error('must not restart') end; function on_reload() assert(ecs==nil); assert(state.text=='hello' and state.flag and state.vector.y==2); state.reloaded=1 end; function on_update(dt) state.count=state.count+10 end");
	ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "starts"), 1.);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "count"), 11.);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "reloaded"), 1.);
}

TEST_F(LuaRuntimeTest, automatic_reload_polls_at_half_second_boundaries)
{
	Load("function on_update(dt) state.value=1 end");
	m_runtime->Update(0.f);
	Write("function on_update(dt) state.value=2 end");
	m_runtime->Update(0.25f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "value"), 1.);
	m_runtime->Update(0.25f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "value"), 2.);
}

TEST_F(LuaRuntimeTest, repaired_source_recovers_a_faulted_script)
{
	Load("function on_update(dt) error('bad update') end");
	m_runtime->Update(0.f);
	ASSERT_FALSE(m_runtime->IsReady());
	Write("function on_update(dt) state.recovered=1 end");
	m_runtime->Update(0.5f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "recovered"), 1.);
}

TEST_F(LuaRuntimeTest, missing_reload_source_does_not_stop_active_code)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	m_runtime->Update(0.f);
	std::filesystem::remove(m_root / "script.lua");
	EXPECT_FALSE(m_runtime->Reload());
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "count"), 2.);
}

TEST_F(LuaRuntimeTest, failed_reload_hook_does_not_commit_candidate_state)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	m_runtime->Update(0.f);
	Write("function on_reload() state.count=999; error('reject') end");
	EXPECT_FALSE(m_runtime->Reload());
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "count"), 1.);
}

TEST_F(LuaRuntimeTest, rejects_bytecode_and_oversized_sources)
{
	Write(std::string("\x1bLua", 4));
	LuaRuntime runtime(m_scene, m_entity.GetUUID(), m_files, "lua:/script.lua");
	EXPECT_FALSE(runtime.Reload());
	Write(std::string(LuaRuntime::MaxSourceBytes + 1, ' '));
	EXPECT_FALSE(runtime.Reload());
}

TEST_F(LuaRuntimeTest, rejects_unmounted_noncanonical_and_nonlua_paths)
{
	Write("state.ok=1");
	std::string error;
	for (const auto* path : { "script.lua", "lua:/../script.lua", "lua:/./script.lua", "lua:/script.txt", "missing:/script.lua" })
	{
		SCOPED_TRACE(path);
		EXPECT_FALSE(LuaRuntime::ValidateSource(m_files, path, {}, error));
	}
}

TEST_F(LuaRuntimeTest, rejects_invalid_callbacks_and_nonflat_reload_state)
{
	Write("on_update=42");
	std::string error;
	EXPECT_FALSE(LuaRuntime::ValidateSource(m_files, "lua:/script.lua", {}, error));
	Write("state.nested={ value=1 }");
	EXPECT_FALSE(LuaRuntime::ValidateSource(m_files, "lua:/script.lua", {}, error));
	Write("state.value=0/0");
	EXPECT_FALSE(LuaRuntime::ValidateSource(m_files, "lua:/script.lua", {}, error));
}

TEST_F(LuaRuntimeTest, validates_authored_property_bounds)
{
	std::string error;
	EXPECT_FALSE(LuaRuntime::ValidateProperties({ { "bad", std::numeric_limits<double>::infinity() } }, error));
	EXPECT_FALSE(LuaRuntime::ValidateProperties({ { "bad", std::string(4097, 'x') } }, error));
	EXPECT_FALSE(LuaRuntime::ValidateProperties({ { "", 1. } }, error));
	LuaRuntime::Properties properties;
	for (int index = 0; index < 129; ++index) properties[std::to_string(index)] = 1.;
	EXPECT_FALSE(LuaRuntime::ValidateProperties(properties, error));
}

TEST_F(LuaRuntimeTest, ignores_invalid_delta_times)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	m_runtime->Update(-1.f);
	m_runtime->Update(std::numeric_limits<float>::quiet_NaN());
	EXPECT_TRUE(m_runtime->GetState().empty());
}

TEST_F(LuaRuntimeTest, rejects_runtime_access_from_other_threads)
{
	Load("function on_update(dt) state.count=(state.count or 0)+1 end");
	bool reloaded = true;
	std::thread worker([&] { reloaded = m_runtime->Reload(); m_runtime->Update(0.f); });
	worker.join();
	EXPECT_FALSE(reloaded);
	EXPECT_TRUE(m_runtime->GetState().empty());
}

TEST_F(LuaRuntimeTest, scene_events_are_deferred_and_cannot_recurse)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0; assert(events.emit('first','payload')) end; function on_event(e) assert(e.type=='GameMessage'); assert(e.sender==ecs.self); assert(e.payload=='payload'); state.events=state.events+1; if state.events==1 then assert(events.emit('second','payload')) end end");
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 0.);
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 1.);
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 2.);
}

TEST_F(LuaRuntimeTest, messages_do_not_leak_between_scenes)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0 end; function on_event(e) state.events=state.events+1 end");
	m_scene.UpdateScripts({});
	Scene other;
	ASSERT_TRUE(other.QueueScriptEvent(std::make_unique<GameEvent>("other")));
	other.UpdateScripts({});
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 0.);
}

TEST_F(LuaRuntimeTest, queue_accepts_worker_posts_and_enforces_backpressure)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0 end; function on_event(e) state.events=state.events+1 end");
	m_scene.UpdateScripts({});
	bool accepted = true;
	std::thread worker([&]
	{
		for (int index = 0; index < 256; ++index)
			accepted = m_scene.QueueScriptEvent(std::make_unique<GameEvent>("worker")) && accepted;
	});
	worker.join();
	EXPECT_TRUE(accepted);
	EXPECT_FALSE(m_scene.QueueScriptEvent(std::make_unique<GameEvent>("overflow")));
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 256.);
	EXPECT_TRUE(m_scene.QueueScriptEvent(std::make_unique<GameEvent>("next")));
}

TEST_F(LuaRuntimeTest, event_binding_rejects_oversized_messages)
{
	Load("function on_update(dt) assert(not events.emit('', '')); assert(not events.emit(string.rep('x',129),'')); assert(not events.emit('topic',string.rep('x',4097))); state.ok=1 end");
	m_runtime->Update(0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "ok"), 1.);
}

TEST_F(LuaRuntimeTest, forwards_copied_keyboard_mouse_and_gamepad_payloads)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0 end; function on_event(e) if e.type=='KeyPressed' then assert(e.key==65 and e.repeat_count==2) elseif e.type=='MouseMoved' then assert(e.x==12 and e.y==34) elseif e.type=='GamepadPressed' then assert(e.button==1 and e.repeat_count==3) else error('unexpected') end; state.events=state.events+1 end");
	m_scene.UpdateScripts({});
	ASSERT_TRUE(m_scene.QueueScriptEvent(std::make_unique<KeyPressedEvent>(65, 2)));
	ASSERT_TRUE(m_scene.QueueScriptEvent(std::make_unique<MouseMovedEvent>(12.f, 34.f)));
	ASSERT_TRUE(m_scene.QueueScriptEvent(std::make_unique<GpadPressedEvent>(1, 3)));
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 3.);
	EXPECT_TRUE(script->GetRuntime().IsReady()) << script->GetRuntime().GetLastError();
}

TEST_F(LuaRuntimeTest, runtime_stop_calls_hook_and_discards_queued_events)
{
	auto* script = Attach(m_entity, "function on_start() state.events=0 end; function on_stop() assert(ecs.set_position(ecs.self,vec3(5,0,0))); events.emit('stale','') end; function on_event(e) state.events=state.events+1 end");
	m_scene.UpdateScripts({});
	ASSERT_TRUE(m_scene.QueueScriptEvent(std::make_unique<GameEvent>("old")));
	m_scene.OnRuntimeStop();
	EXPECT_FALSE(script->GetRuntime().IsReady());
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 5.f);
	m_scene.UpdateScripts({});
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(script->GetRuntime(), "events"), 0.);
}

TEST_F(LuaRuntimeTest, removing_script_runs_its_stop_hook)
{
	Attach(m_entity, "function on_stop() ecs.set_position(ecs.self,vec3(7,0,0)) end");
	m_scene.UpdateScripts({});
	m_entity.GetComponent<NativeScriptComponent>().RemoveScript<LuaScript>();
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 7.f);
}

TEST_F(LuaRuntimeTest, one_faulted_script_does_not_stop_other_entities)
{
	auto* broken = Attach(m_entity, "function on_update(dt) error('bad') end");
	auto other = m_scene.CreateEntity();
	Write("function on_update(dt) state.count=(state.count or 0)+1 end", "other.lua");
	other.AddComponent<NativeScriptComponent>();
	auto record = Record();
	record.properties["asset"] = std::string("lua:/other.lua");
	auto* healthy = other.GetComponent<NativeScriptComponent>().AddScript<LuaScript>(other, m_files, record);
	m_scene.UpdateScripts({});
	m_scene.UpdateScripts({});
	EXPECT_FALSE(broken->GetRuntime().IsReady());
	EXPECT_DOUBLE_EQ(Number(healthy->GetRuntime(), "count"), 2.);
}

TEST_F(LuaRuntimeTest, persistence_round_trip_retains_authored_properties_not_runtime_state)
{
	Write("function on_update(dt) state.speed=999; state.transient=42 end");
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	Scene scene;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(Definition(), scene, ".", registry, error)) << error;
	scene.UpdateScripts({});
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, ".", registry, captured, error)) << error;
	ASSERT_EQ(captured.entities.size(), 1u);
	ASSERT_EQ(captured.entities[0].scripts.size(), 1u);
	const auto& properties = captured.entities[0].scripts[0].properties;
	EXPECT_DOUBLE_EQ(std::get<double>(properties.at("speed")), 2.5);
	EXPECT_EQ(std::get<std::string>(properties.at("asset")), "lua:/script.lua");
	EXPECT_EQ(properties.count("transient"), 0u);
	std::string json;
	ASSERT_TRUE(SceneJson::Serialize(captured, json, error)) << error;
	SceneRecord parsed;
	ASSERT_TRUE(SceneJson::Parse(json, parsed, error)) << error;
	Scene restored;
	ASSERT_TRUE(SceneSerializer::Instantiate(parsed, restored, ".", registry, error)) << error;
}

TEST_F(LuaRuntimeTest, invalid_lua_records_fail_before_scene_construction)
{
	Write("function on_update(");
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	Scene scene;
	scene.SetSceneName("Unchanged");
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(Definition(), scene, ".", registry, error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetSceneName(), "Unchanged");
	Write("state.ok=1");
	auto record = Definition();
	record.entities[0].scripts[0].properties["asset"] = 1.;
	EXPECT_FALSE(SceneSerializer::Instantiate(record, scene, ".", registry, error));
	EXPECT_TRUE(scene.IsEmpty());
}

TEST_F(LuaRuntimeTest, registry_rejects_duplicate_lua_adapter_registration)
{
	ScriptRegistry registry;
	EXPECT_TRUE(LuaScript::Register(registry, m_files));
	EXPECT_FALSE(LuaScript::Register(registry, m_files));
}

TEST_F(LuaRuntimeTest, factory_failure_detaches_only_failed_lua_script)
{
	Write("function on_update(dt) state.value=1 end");
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	std::string error;
	ASSERT_TRUE(registry.Validate(Definition().entities[0], error)) << error;
	auto& component = m_entity.AddComponent<NativeScriptComponent>();
	auto* original = component.AddScript<NativeScript>(m_entity);
	Write("function on_update(");
	EXPECT_FALSE(registry.Instantiate(m_entity, Record(), error));
	EXPECT_FALSE(component.ScriptExists<LuaScript>());
	EXPECT_EQ(component.native_scripts.size(), 1u);
	EXPECT_EQ(component.GetScript<NativeScript>(), original);
	EXPECT_TRUE(m_scene.GetEntityByUUID(m_entity.GetUUID()));
}

TEST_F(LuaRuntimeTest, duplicate_factory_call_preserves_existing_lua_script)
{
	Write("function on_update(dt) state.value=1 end");
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	auto& component = m_entity.AddComponent<NativeScriptComponent>();
	std::string error;
	ASSERT_TRUE(registry.Instantiate(m_entity, Record(), error)) << error;
	auto* original = component.GetScript<LuaScript>();
	EXPECT_FALSE(registry.Instantiate(m_entity, Record(), error));
	EXPECT_EQ(component.GetScript<LuaScript>(), original);
	EXPECT_EQ(component.native_scripts.size(), 1u);
	m_scene.UpdateScripts({});
	EXPECT_DOUBLE_EQ(Number(original->GetRuntime(), "value"), 1.);
}

TEST_F(LuaRuntimeTest, prefab_spawning_applies_lua_overrides_and_captures_expanded_records)
{
	Write("function on_start() assert(ecs.set_position(ecs.self,vec3(properties.speed,0,0))) end");
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	std::string error;
	ASSERT_TRUE(SceneJson::Save(m_root / "prefab.scene.json", Definition(), error)) << error;
	PrefabRecord prefab;
	prefab.path = "prefab.scene.json";
	EntityOverrideRecord replacement;
	replacement.id = "42";
	auto script = Record();
	script.properties["speed"] = 9.;
	replacement.scripts = std::vector<ScriptRecord>{ script };
	prefab.overrides.push_back(replacement);
	Daedalus::UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(prefab, m_scene, m_root, registry, instance, error)) << error;
	m_scene.UpdateScripts({});
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(m_scene, m_root, registry, captured, error)) << error;
	bool found = false;
	for (const auto& entity : captured.entities)
	{
		EXPECT_FALSE(entity.prefab.has_value());
		if (entity.scripts.empty()) continue;
		found = true;
		EXPECT_NE(entity.id, "42");
		EXPECT_FLOAT_EQ(entity.transform.translation.x, 9.f);
		EXPECT_DOUBLE_EQ(std::get<double>(entity.scripts[0].properties.at("speed")), 9.);
	}
	EXPECT_TRUE(found);
	EXPECT_FALSE(captured.base_prefab.has_value());
}

TEST_F(LuaRuntimeTest, loads_lua_sources_from_zip_mounts)
{
	const PathDefaults restore_paths;
	const std::string source = "function on_update(dt) state.value=42 end";
	const auto path = m_root / "scripts.zip";
	int code = 0;
	std::unique_ptr<zip_t, decltype(&zip_discard)> archive(zip_open(path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &code), &zip_discard);
	ASSERT_NE(archive, nullptr);
	auto* buffer = zip_source_buffer(archive.get(), source.data(), source.size(), 0);
	ASSERT_NE(buffer, nullptr);
	ASSERT_GE(zip_file_add(archive.get(), "script.lua", buffer, ZIP_FL_ENC_UTF_8), 0);
	ASSERT_EQ(zip_close(archive.get()), 0);
	archive.release();
	Write("[Path]\nScriptsFolder=scripts.zip\n", "path.ini");
	PathConfig::Load(m_root / "path.ini");
	VirtualFileSystem files;
	std::string error;
	ASSERT_TRUE(files.Mount("scripts", PathConfig::GetScriptsPath(), error)) << error;
	EXPECT_TRUE(files.IsArchive("scripts"));
	LuaRuntime runtime(m_scene, m_entity.GetUUID(), files, "scripts:/script.lua");
	runtime.Update(0.f);
	EXPECT_DOUBLE_EQ(Number(runtime, "value"), 42.);
}

TEST_F(LuaRuntimeTest, bounded_event_engine_preserves_existing_deferred_dispatch_contract)
{
	EventEngine events(1);
	events.Start();
	EXPECT_TRUE(events.AddEvent(std::make_unique<GameEvent>("first")));
	EXPECT_FALSE(events.AddEvent(std::make_unique<GameEvent>("overflow")));
	int received = 0;
	events.DispatchPending([&](Event&)
	{
		++received;
		EXPECT_TRUE(events.AddEvent(std::make_unique<GameEvent>("next")));
	});
	EXPECT_EQ(received, 1);
	events.DispatchPending([&](Event&) { ++received; });
	EXPECT_EQ(received, 2);
}

TEST_F(LuaRuntimeTest, gameplay_world_pose_bindings_preserve_parent_space_and_scale)
{
	auto parent = m_scene.CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().translation = { 10.f, 0.f, 0.f };
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	ASSERT_TRUE(m_scene.SetParent(m_entity.GetUUID(), parent.GetUUID()));
	m_entity.GetComponent<TransformComponent>().scale = glm::vec3(2.f);
	Load(R"(
		assert(camera == nil and log == nil)
		function on_update(dt)
			assert(ecs.parent(ecs.self) ~= "0")
			assert(ecs.set_world_pose(ecs.self, vec3(3,4,5), vec3(0,0,0)))
			local p = ecs.world_position(ecs.self)
			state.x = p.x; state.y = p.y; state.z = p.z
			state.yaw = ecs.world_rotation(ecs.self).y
			state.local_x = ecs.local_direction(ecs.parent(ecs.self), vec3(0,0,-1)).x
			assert(ecs.set_world_pose(ecs.self, p, vec3(0,0,0), 180))
			state.forward_z = ecs.forward(ecs.self).z
		end
	)");
	m_runtime->Update(0.f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_NEAR(Number(*m_runtime, "x"), 3., 0.0001);
	EXPECT_NEAR(Number(*m_runtime, "y"), 4., 0.0001);
	EXPECT_NEAR(Number(*m_runtime, "z"), 5., 0.0001);
	EXPECT_NEAR(Number(*m_runtime, "yaw"), 0., 0.0001);
	EXPECT_NEAR(Number(*m_runtime, "local_x"), 1., 0.0001);
	EXPECT_NEAR(glm::distance(m_entity.GetComponent<TransformComponent>().scale, glm::vec3(2.f)), 0.f, 0.0001f);
}

TEST_F(LuaRuntimeTest, gameplay_camera_bindings_update_world_pose_under_a_parent)
{
	auto parent = m_scene.CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	ASSERT_TRUE(m_scene.SetParent(m_entity.GetUUID(), parent.GetUUID()));
	auto& camera = *m_entity.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	camera.SetMovementSpeed(4.f);
	camera.SetSensivitity(1.f);
	Load(R"(
		function on_update(dt)
			local direction = camera.direction(ecs.self)
			local right = camera.right(ecs.self)
			assert(direction.x < -0.99 and right.z < -0.99)
			assert(camera.move(ecs.self, vec3(0,0,1), dt))
			assert(camera.set_person(ecs.self, 7, 4))
			state.x = camera.position(ecs.self).x
			assert(camera.look(ecs.self, 10, 5))
		end
	)");
	m_runtime->Update(0.5f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_NEAR(Number(*m_runtime, "x"), -2., 0.0001);
	EXPECT_FLOAT_EQ(camera.GetPersonDistance(), 7.f);
	EXPECT_FLOAT_EQ(camera.GetPersonHeight(), 4.f);
	EXPECT_NEAR(camera.GetPitch(), 5.f, 0.0001f);
	EXPECT_NEAR(std::remainder(camera.GetYaw() + 170.f, 360.f), 0.f, 0.0001f);
}

TEST_F(LuaRuntimeTest, gameplay_bindings_reject_invalid_values_and_missing_components)
{
	m_entity.AddComponent<RenderableObjectComponent>();
	LightProps props;
	props.light_power = 4.f;
	m_entity.AddComponent<SpotLightComponent>(props);
	m_entity.AddComponent<CameraComponent>(CameraProjectionProps{});
	Load(R"(
		function on_update(dt)
			local id = ecs.self
			assert(not ecs.set_visible(id, 1))
			assert(not ecs.set_visible("0", true))
			assert(not ecs.set_spotlight_power(id, -1))
			assert(not ecs.set_spotlight_power(id, 0/0))
			assert(not ecs.set_spotlight_power(id, "5"))
			assert(ecs.spotlight_power("0") == nil)
			assert(ecs.world_position({}) == nil and ecs.world_rotation("0") == nil)
			assert(ecs.forward("0") == nil and ecs.parent(false) == "0")
			assert(ecs.local_direction("0", vec3(1,2,3)).x == 1)
			assert(ecs.local_direction("999", vec3(1,2,3)) == nil)
			assert(ecs.local_direction(id, {}) == nil)
			assert(not ecs.set_world_pose(id, vec3(0/0,0,0), vec3(0,0,0)))
			assert(not ecs.set_world_pose(id, vec3(0,0,0), vec3(0,0,0), "bad"))
			assert(not camera.move(id, {}, 1))
			assert(not camera.move(id, vec3(0,0,1), -1))
			assert(not camera.move(id, vec3(0,0,1), math.huge))
			assert(not camera.look(id, "1", 0))
			assert(not camera.look(id, 0, 0/0))
			assert(not camera.set_person(id, -1, 0))
			assert(not camera.set_person(id, 0, {}))
			assert(camera.position("0") == nil and camera.right(false) == nil)
			assert(camera.direction("999") == nil)
			assert(not camera.move("0", vec3(0,0,0), 0))
			assert(not camera.look("0", 0, 0))
			assert(not camera.set_person("0", 0, 0))
			assert(not log.trace({}) and not log.trace(string.rep("x",4097)))
			assert(input.movement({}, "MoveLeft") == 0)
		end
	)");
	m_runtime->Update(0.f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_TRUE(m_entity.GetComponent<RenderableObjectComponent>().visible);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<SpotLightComponent>().light.GetProps().light_power, 4.f);
	EXPECT_EQ(m_entity.GetComponent<TransformComponent>().translation, glm::vec3(0.f));
}

TEST_F(LuaRuntimeTest, gameplay_visibility_and_power_do_not_remove_authored_components)
{
	m_entity.AddComponent<RenderableObjectComponent>();
	LightProps props;
	props.light_power = 4.f;
	m_entity.AddComponent<SpotLightComponent>(props);
	Load(R"(
		function on_update(dt)
			assert(ecs.set_visible(ecs.self, false))
			state.power = ecs.spotlight_power(ecs.self)
			assert(ecs.set_spotlight_power(ecs.self, 0))
		end
	)");
	m_runtime->Update(0.f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_FALSE(m_entity.GetComponent<RenderableObjectComponent>().visible);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "power"), 4.);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<SpotLightComponent>().light.GetProps().light_power, 0.f);
}

TEST_F(LuaRuntimeTest, gameplay_input_bindings_are_neutral_without_input_service)
{
	ASSERT_FALSE(Input::IsAvailable());
	Load(R"(
		function on_update(dt)
			assert(input.movement("MoveForward", "MoveBackward") == 0)
			assert(not input.gamepad_enabled())
			local mouse = input.mouse_position()
			assert(mouse.x == 0 and mouse.y == 0)
			assert(log.trace("Lua headless gameplay bindings"))
		end
	)");
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
}

TEST_F(LuaRuntimeTest, sandbox_scenes_reference_only_valid_lua_example_assets)
{
	ASSERT_TRUE(m_files.Mount("scripts", std::filesystem::path(DL_SANDBOX_EXAMPLES) / "scripts"));
	ScriptRegistry registry;
	ASSERT_TRUE(LuaScript::Register(registry, m_files));
	for (const auto* name : { "nuke.scene.json", "anor_londo.scene.json" })
	{
		SCOPED_TRACE(name);
		SceneRecord record;
		std::string error;
		ASSERT_TRUE(SceneJson::Load(std::filesystem::path(DL_SANDBOX_EXAMPLES) / "scenes" / name, record, error)) << error;
		for (const auto& entity : record.entities)
		{
			ASSERT_TRUE(registry.Validate(entity, error)) << error;
			for (const auto& script : entity.scripts) EXPECT_EQ(script.type, "Lua");
		}
	}
}

TEST_F(LuaRuntimeTest, sandbox_rotation_example_preserves_native_speed_units)
{
	m_entity.GetComponent<TransformComponent>().rotation = { 10.f, 20.f, 30.f };
	LoadExample("rotation_model.lua", { { "speed", 0.1 } });
	m_runtime->Update(0.25f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().rotation.y, 45.f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().rotation.x, 10.f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().rotation.z, 30.f);
}

TEST_F(LuaRuntimeTest, sandbox_tank_example_completes_patrol_and_preserves_reload_state)
{
	LoadExample("tank_movement.lua", { { "distance", 5. }, { "forward_direction", glm::vec3(1.f, 0.f, 0.f) } });
	m_runtime->Update(1.f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 5.f);
	ASSERT_TRUE(m_runtime->Reload()) << m_runtime->GetLastError();
	for (int i = 0; i < 3; ++i) m_runtime->Update(1.f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().rotation.y, -180.f);
	for (int i = 0; i < 4; ++i) m_runtime->Update(1.f);
	ASSERT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().translation.x, 0.f);
	EXPECT_FLOAT_EQ(m_entity.GetComponent<TransformComponent>().rotation.y, 0.f);
	EXPECT_DOUBLE_EQ(Number(*m_runtime, "length"), 0.);
}

TEST_F(LuaRuntimeTest, sandbox_position_logging_supports_self_linked_and_missing_cameras)
{
	LoadExample("log_position.lua");
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	auto camera = m_scene.CreateEntity("Camera");
	camera.AddComponent<CameraComponent>(CameraProjectionProps{});
	ASSERT_TRUE(m_scene.SetEntityLink(m_entity.GetUUID(), "camera", camera.GetUUID()));
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	m_entity.AddComponent<CameraComponent>(CameraProjectionProps{});
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
	m_scene.DestroyEntity(camera);
	m_entity.RemoveComponent<CameraComponent>();
	m_runtime->Update(0.f);
	EXPECT_TRUE(m_runtime->IsReady()) << m_runtime->GetLastError();
}
