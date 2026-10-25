#include <ECS/Entity.h>
#include <ECS/NativeScript.h>
#include <ECS/NativeScriptComponent.h>
#include <ECS/Scene.h>

#include <gtest/gtest.h>

using namespace Daedalus;

namespace
{
	class CounterScript : public NativeScript
	{
	public:
		CounterScript(Entity entity, int start = 0) : NativeScript(entity), value(start) {}

		void Tick(DeltaTime dt) { OnUpdate(dt); }
		TagComponent& Tag() { return GetComponent<TagComponent>(); }

		int value;

	protected:
		void OnUpdate(DeltaTime) override { ++value; }
	};

	class OtherScript : public NativeScript
	{
	public:
		explicit OtherScript(Entity entity) : NativeScript(entity) {}
	};
}

TEST(NativeScriptTest, add_script_returns_instance_and_forwards_arguments)
{
	Scene scene;
	Entity entity = scene.CreateEntity("Scripted");
	auto& nsc = entity.AddComponent<NativeScriptComponent>();

	CounterScript* script = nsc.AddScript<CounterScript>(entity, 5);

	ASSERT_NE(script, nullptr);
	EXPECT_EQ(script->value, 5);
	EXPECT_TRUE(nsc.ScriptExists<CounterScript>());
	EXPECT_FALSE(nsc.ScriptExists<OtherScript>());
}

TEST(NativeScriptTest, get_script_returns_same_instance)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	auto& nsc = entity.AddComponent<NativeScriptComponent>();

	CounterScript* added = nsc.AddScript<CounterScript>(entity);

	EXPECT_EQ(nsc.GetScript<CounterScript>(), added);
}

TEST(NativeScriptTest, remove_script_deletes_it)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	auto& nsc = entity.AddComponent<NativeScriptComponent>();
	nsc.AddScript<CounterScript>(entity);

	nsc.RemoveScript<CounterScript>();

	EXPECT_FALSE(nsc.ScriptExists<CounterScript>());
}

TEST(NativeScriptTest, multiple_script_types_coexist)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	auto& nsc = entity.AddComponent<NativeScriptComponent>();

	nsc.AddScript<CounterScript>(entity);
	nsc.AddScript<OtherScript>(entity);

	EXPECT_TRUE(nsc.ScriptExists<CounterScript>());
	EXPECT_TRUE(nsc.ScriptExists<OtherScript>());
	EXPECT_EQ(nsc.native_scripts.size(), 2u);
}

TEST(NativeScriptTest, script_accesses_entity_components)
{
	Scene scene;
	Entity entity = scene.CreateEntity("Hero");
	auto& nsc = entity.AddComponent<NativeScriptComponent>();
	CounterScript* script = nsc.AddScript<CounterScript>(entity);

	EXPECT_EQ(script->Tag().tag, "Hero");
}

TEST(NativeScriptTest, on_update_is_invoked)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	auto& nsc = entity.AddComponent<NativeScriptComponent>();
	CounterScript* script = nsc.AddScript<CounterScript>(entity);

	script->Tick(DeltaTime());
	script->Tick(DeltaTime());

	EXPECT_EQ(script->value, 2);
}
