#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

#include "ExampleScripts.h"

using namespace Daedalus;

namespace
{
	ModelParserFlags enhance_model_flags =
		ModelParserFlags::JOIN_IDENTICAL_VERTICES |
		ModelParserFlags::TRIANGULATE |
		ModelParserFlags::GEN_SMOOTH_NORMALS |
		ModelParserFlags::SPLIT_LARGE_MESHES |
		ModelParserFlags::OPTIMIZE_MESHES |
		ModelParserFlags::OPTIMIZE_GRAPH |
		ModelParserFlags::CALC_TANGENT_SPACE;

	//Create complex level from multiple related obj files
	void CreateEntitiesForOBJFiles(const std::string& folderPath, Scene& scene, const TransformComponent& transform)
	{
		namespace fs = std::filesystem;

		for (const auto& entry : fs::directory_iterator(folderPath))
		{
			if (entry.path().extension() == ".obj")
			{
				std::string filename = entry.path().stem().string();

				auto model_entity = scene.CreateEntity(filename);
				model_entity.AddComponent<RenderableObjectComponent>(entry.path().string(), enhance_model_flags);

				auto& model_transform = model_entity.GetComponent<TransformComponent>();
				model_transform = transform;
			}
		}
	}
	//CreateEntitiesForOBJFiles("C:/maxym/objects/Anor Londo/", m_scene, model_transform);


	void InstanceDummyLights(Scene& scene)
	{
		auto light_entity1 = scene.CreateEntity("Main Light 1");
		light_entity1.AddComponent<DirectionalLightComponent>(glm::vec3(1.8f, 1.f, 0.6f),
			glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 0.5f, false);

		auto light_entity2 = scene.CreateEntity("Main Light 2");
		light_entity2.AddComponent<DirectionalLightComponent>(glm::vec3(2.8f, 1.f, 0.6f),
			glm::vec3(2.f, 1.f, 1.f), glm::vec3(1.f, 2.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 2.f, false);

		auto light_entity3 = scene.CreateEntity("Main Light 3 ");
		light_entity3.AddComponent<DirectionalLightComponent>(glm::vec3(3.8f, 1.f, 0.6f),
			glm::vec3(3.f, 1.f, 1.f), glm::vec3(1.f, 3.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 3.f, false);

		auto light_entity4 = scene.CreateEntity("Main Light 4");
		light_entity4.AddComponent<DirectionalLightComponent>(glm::vec3(4.8f, 1.f, 0.6f),
			glm::vec3(4.f, 1.f, 1.f), glm::vec3(1.f, 4.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 4.f, false);

		auto light_entity5 = scene.CreateEntity("Main Light 5 ");
		light_entity5.AddComponent<PointLightComponent>(glm::vec3(5.8f, 1.f, 0.6f),
			glm::vec3(5.f, 1.f, 1.f), glm::vec3(1.f, 5.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 5.f, 10.f, false);

		auto light_entity6 = scene.CreateEntity("Main Light 6");
		light_entity6.AddComponent<PointLightComponent>(glm::vec3(6.8f, 1.f, 0.6f),
			glm::vec3(6.f, 1.f, 1.f), glm::vec3(1.f, 6.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 6.f, 15.f, false);

		auto light_entity7 = scene.CreateEntity("Main Light 7");
		light_entity7.AddComponent<SpotLightComponent>(glm::vec3(7.8f, 1.f, 0.6f),
			glm::vec3(7.f, 1.f, 1.f), glm::vec3(1.f, 7.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 7.f, 13.f, 70.f, false);

		auto light_entity8 = scene.CreateEntity("Main Light 8");
		light_entity8.AddComponent<SpotLightComponent>(glm::vec3(8.8f, 1.f, 0.6f),
			glm::vec3(8.f, 1.f, 1.f), glm::vec3(1.f, 8.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 8.f, 15.f, 15.f, false);

		auto light_entity9 = scene.CreateEntity("Main Light 9");
		light_entity9.AddComponent<SpotLightComponent>(glm::vec3(9.8f, 1.f, 0.6f),
			glm::vec3(9.f, 1.f, 1.f), glm::vec3(1.f, 9.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 9.f, 23.f, 39.f, false);

		auto light_entity10 = scene.CreateEntity("Main Light 10");
		light_entity10.AddComponent<SpotLightComponent>(glm::vec3(10.8f, 1.f, 0.6f),
			glm::vec3(10.f, 1.f, 1.f), glm::vec3(1.f, 10.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 10.f, 27.f, 42.f, false);

		auto light_entity11 = scene.CreateEntity("Main Light 11");
		light_entity11.AddComponent<SpotLightComponent>(glm::vec3(11.8f, 1.f, 0.6f),
			glm::vec3(11.f, 1.f, 1.f), glm::vec3(1.f, 11.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 11.f, 34.f, 43.f, false);

		auto light_entity12 = scene.CreateEntity("Main Light 12");
		light_entity12.AddComponent<SpotLightComponent>(glm::vec3(12.8f, 1.f, 0.6f),
			glm::vec3(12.f, 1.f, 1.f), glm::vec3(1.f, 12.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 12.f, 45.f, 42.f, false);
	}

}

void PrepareNukeScene(Scene& scene);
void PrepareAnorLondoScene(Scene& scene);

void ExampleLayer::OnAttach()
{
	PrepareNukeScene(m_scene);
	//PrepareAnorLondoScene(m_scene);

	m_scene.OnRuntimeStart();
}

void ExampleLayer::OnDetach()
{
	m_scene.OnRuntimeStop();
}

void ExampleLayer::OnUpdate(DeltaTime dt)
{
	m_scene.OnUpdateRuntime(dt);
}

void PrepareNukeScene(Scene& scene)
{
	auto camera_entity = scene.CreateEntity("Main Camera");
	camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, Application::GetInstance()->GetWindow().GetAspectRatio(), 0.1f, 1000.f));
	auto& camera = camera_entity.GetComponent<CameraComponent>().camera;
	camera.SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera.RotateCamera(-90.f, 0.f);
	camera.SetMovementSpeed(15.f);
	camera.SetSensivitity(5.f);
	camera.SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	camera_scripts.AddScript<LogPositionScript>(camera_entity);

	InstanceDummyLights(scene);
	auto light_entity = scene.CreateEntity("Main Light");
	const auto& light_component = light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.8f, 1.f, 0.6f),
		glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 3.f, false);

	const auto standard_shader = Renderer::s_shader_library->Get(ShaderConstants::StandardShader);
	standard_shader->Bind();

	const auto& params = light_component.light.GetShaderSSBO();
	standard_shader->SetFloat3(ShaderConstants::LightPos, glm::vec3(params.position[0], params.position[1], params.position[2]));
	standard_shader->SetFloat3(ShaderConstants::LightAmbient, glm::vec3(params.ambient_color[0], params.ambient_color[1], params.ambient_color[2]));
	standard_shader->SetFloat3(ShaderConstants::LightDiffuse, glm::vec3(params.diffuse_color[0], params.diffuse_color[1], params.diffuse_color[2]));
	standard_shader->SetFloat3(ShaderConstants::LightSpecular, glm::vec3(params.specular_color[0], params.specular_color[1], params.specular_color[2]));
	standard_shader->SetFloat(ShaderConstants::LightPower, params.power);

	auto nuke_entity = scene.CreateEntity("Nuke");
	nuke_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Nuke/Nuke.obj", enhance_model_flags);
	auto& nuke_transfrom = nuke_entity.GetComponent<TransformComponent>();
	nuke_transfrom.scale = glm::vec3(2.f, 2.f, 2.f);
	nuke_transfrom.translation = glm::vec3(-50.f, 18.f, -120.f);
	nuke_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);

	auto kratos_model = scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj", enhance_model_flags);
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(0.f, -2.8f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_entity = scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", enhance_model_flags);
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-10.f, -2.8f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RotationModelScript>(miranda_entity, 0.1f);

	auto tank_entity = scene.CreateEntity("Tank");
	tank_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj", enhance_model_flags);
	auto& tank_transform = tank_entity.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(3.f, 3.f, 3.f);
	tank_transform.translation = glm::vec3(-20.f, -2.8f, -25.f);
	tank_transform.rotation = glm::vec3(0.f, 90.f, 0.f);
	auto& tank_scripts = tank_entity.AddComponent<NativeScriptComponent>();
	tank_scripts.AddScript<TankMovementScript>(tank_entity, glm::vec3(1.0f, 0.f, 0.0f), 50.f);
}

void PrepareAnorLondoScene(Scene& scene)
{
	auto camera_entity = scene.CreateEntity("Main Camera");
	camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, Application::GetInstance()->GetWindow().GetAspectRatio(), 0.1f, 10000.f));
	auto& camera = camera_entity.GetComponent<CameraComponent>().camera;
	camera.SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera.RotateCamera(-90.f, 0.f);
	camera.SetMovementSpeed(15.f);
	camera.SetSensivitity(5.f);
	camera.SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	camera_scripts.AddScript<LogPositionScript>(camera_entity);

	auto light_entity = scene.CreateEntity("Main Light");
	light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.3f, 1.f, 1.2f),
		glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.95f, 0.85f, 0.65f), 1.f, false);

	TransformComponent transform;
	transform.scale = glm::vec3(2.f, 2.f, 2.f);
	transform.translation = glm::vec3(510.f, -240.f, 730.f);
	transform.rotation = glm::vec3(0.f, -90.f, 0.f);
	CreateEntitiesForOBJFiles(WorkingDirectory::GetAssetsDirectory().string() + "/models/Anor Londo/", scene, transform);

	auto kratos_model = scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj", enhance_model_flags);
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(9.f, -6.f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_entity = scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", enhance_model_flags);
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-9.f, -6.f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RotationModelScript>(miranda_entity, 0.1f);

	auto marci_entity = scene.CreateEntity("Marci");
	marci_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Marci/marci.fbx", enhance_model_flags);
	auto& marci_transform = marci_entity.GetComponent<TransformComponent>();
	marci_transform.scale = glm::vec3(0.0002f, 0.0002f, 0.0002f);
	marci_transform.translation = glm::vec3(-9.f, -6.f, -35.f);
	marci_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& marci_scripts = marci_entity.AddComponent<NativeScriptComponent>();
	marci_scripts.AddScript<RotationModelScript>(marci_entity, 0.1f);

	auto tank_entity = scene.CreateEntity("Tank");
	tank_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj", enhance_model_flags);
	auto& tank_transform = tank_entity.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(3.f, 3.f, 3.f);
	tank_transform.translation = glm::vec3(0.f, -5.8f, -20.f);
	tank_transform.rotation = glm::vec3(0.f, 180.f, 0.f);
	auto& tank_scripts = tank_entity.AddComponent<NativeScriptComponent>();
	tank_scripts.AddScript<TankMovementScript>(tank_entity, glm::vec3(0.0f, 0.f, -1.0f), 30.f);
}