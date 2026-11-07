#include <Renderer/Objects/Cubemap.h>
#include <ECS/Entity.h>
#include <ECS/Scene.h>

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace Daedalus
{
	namespace
	{
		std::unique_ptr<Cubemap> CreateClientCubemap(const std::vector<std::string>& faces)
		{
			return std::make_unique<Cubemap>(faces);
		}

		std::unique_ptr<SkyboxComponent> CreateClientSkybox(const std::vector<std::string>& faces, float rotation_angle)
		{
			return std::make_unique<SkyboxComponent>(faces, rotation_angle);
		}

		void ExerciseClientSkyboxLifecycle(const std::vector<std::string>& faces)
		{
			Scene scene;
			auto entity = scene.CreateEntity("skybox boundary");
			auto& component = entity.AddComponent<SkyboxComponent>(faces, 15.f);
			SkyboxComponent copied(component);
			SkyboxComponent moved(std::move(copied));
			copied = moved;
			moved = std::move(copied);
			entity.AddOrReplaceComponent<SkyboxComponent>(moved);
			auto popped = entity.PopComponent<SkyboxComponent>();
			entity.PushComponent(std::move(popped));
			entity.RemoveComponent<SkyboxComponent>();
		}
	}

	// Volatile function pointers retain DLL-link references without executing GPU construction.
	TEST(SkyboxComponentTest, cubemap_face_constructor_links_from_engine_client)
	{
		auto* volatile factory = &CreateClientCubemap;
		EXPECT_NE(factory, nullptr);
	}

	TEST(SkyboxComponentTest, skybox_face_constructor_links_from_engine_client)
	{
		auto* volatile factory = &CreateClientSkybox;
		EXPECT_NE(factory, nullptr);
	}

	TEST(SkyboxComponentTest, ecs_skybox_lifecycle_links_from_engine_client)
	{
		auto* volatile lifecycle = &ExerciseClientSkyboxLifecycle;
		EXPECT_NE(lifecycle, nullptr);
	}

	TEST(SkyboxComponentTest, construction_and_copy_traits_are_unchanged)
	{
		EXPECT_FALSE(std::is_default_constructible_v<Cubemap>);
		EXPECT_FALSE(std::is_default_constructible_v<SkyboxComponent>);
		EXPECT_TRUE((std::is_constructible_v<SkyboxComponent, const std::vector<std::string>&, float>));
		EXPECT_TRUE(std::is_copy_constructible_v<SkyboxComponent>);
		EXPECT_TRUE(std::is_copy_assignable_v<SkyboxComponent>);
		EXPECT_TRUE(std::is_move_constructible_v<SkyboxComponent>);
		EXPECT_TRUE(std::is_move_assignable_v<SkyboxComponent>);
	}
}
