#include "ExampleLayer.h"
#include "Daedalus/ECS/NativeScripts/CameraController.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

#include "ExampleScripts.h"

using namespace Daedalus;

ModelParserFlags ExampleLayer::s_enhance_model_flags =
	ModelParserFlags::JOIN_IDENTICAL_VERTICES |
	ModelParserFlags::TRIANGULATE |
	ModelParserFlags::GEN_SMOOTH_NORMALS |
	ModelParserFlags::SPLIT_LARGE_MESHES |
	ModelParserFlags::OPTIMIZE_MESHES |
	ModelParserFlags::OPTIMIZE_GRAPH |
	ModelParserFlags::CALC_TANGENT_SPACE;

namespace
{
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
				model_entity.AddComponent<RenderableObjectComponent>(entry.path().string(), ExampleLayer::s_enhance_model_flags);

				auto& model_transform = model_entity.GetComponent<TransformComponent>();
				model_transform = transform;
			}
		}
	}
}

void ExampleLayer::OnAttach()
{
	//PrepareNukeScene();
	//PrepareAnorLondoScene();
	PreparePBRScene();

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

void ExampleLayer::PrepareNukeScene()
{
	Daedalus::LightProps props;

	auto camera_entity = m_scene.CreateEntity("Main Camera");

	const auto aspect_ratio = Application::GetInstance()->GetWindow().GetAspectRatio();
	auto& camera_comp = camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, aspect_ratio, 0.1f, 200.f));

	//const auto scale_ortho = 20.f;
	//auto& camera_comp = camera_entity.AddComponent<CameraComponent>(-scale_ortho * aspect_ratio, scale_ortho * aspect_ratio, -scale_ortho, scale_ortho, -scale_ortho * 10.0f, scale_ortho * 10.f);

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto camera = camera_comp.camera.get();
	camera->SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera->SetPersonParams(7.f, 4.f);
	camera->RotateCamera(-90.f, 0.f);
	camera->SetMovementSpeed(15.f);
	camera->SetSensivitity(5.f);
	camera->SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	//camera_scripts.AddScript<LogPositionScript>(camera_entity);
	camera_scripts.AddScript<PlayerControllerScript>(camera_entity);

	camera_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", s_enhance_model_flags);
	auto& miran_transform = camera_entity.GetComponent<TransformComponent>();
	miran_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miran_transform.translation = glm::vec3(0.f, 0.f, 0.f);
	miran_transform.rotation = glm::vec3(0.f, 180.f, 0.f);

	props.light_pos = glm::vec3(0.f, 0.0f, 0.0f);
	props.ambient_color = glm::vec3(0.f, 0.f, 0.f);
	props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
	props.specular_color = glm::vec3(1.f, 1.f, 1.f);
	props.direction = glm::vec3{ 0.f, 0.f, 1.f };
	props.light_power = 4.f;
	props.max_distance = 100.f;
	props.theta_angle = 10.f;
	props.outer_cutoff = 35.f;
	props.cast_shadows = false;
	props.number_of_shadow_cascades = 1;
	camera_entity.AddComponent<SpotLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */
	 auto light_entity = m_scene.CreateEntity("Main Light");
	 props.light_pos = glm::vec3(0.78f, 1.0f, 0.6f);
	 props.ambient_color = glm::vec3(0.2f, 0.2f, 0.2f);
	 props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
	 props.specular_color = glm::vec3(1.f, 1.f, 1.f);
	 props.direction = glm::vec3(0.78f, 1.0f, 0.6f);
	 props.light_power = 0.2f;
	 props.cast_shadows = false;
	 props.number_of_shadow_cascades = 4;
	 light_entity.AddComponent<DirectionalLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */
	auto Aside_light_entity = m_scene.CreateEntity("ASide Light");
	props.light_pos = glm::vec3(-26.0f, 12.f, -74.5f);
	props.ambient_color = glm::vec3(0.1f, 0.1f, 0.1f);
	props.diffuse_color = glm::vec3(1.f, 0.8f, 0.6f);
	props.specular_color = glm::vec3(1.f, 1.f, 0.6f);
	props.direction = glm::vec3(0.79f, -0.5f, 0.35f);
	props.light_power = 30.f;
	props.max_distance = 50.f;
	props.cast_shadows = false;
	props.number_of_shadow_cascades = 1;
	Aside_light_entity.AddComponent<PointLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto spot_entity = m_scene.CreateEntity("Spot Light");
	spot_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Light/Light.blend", ExampleLayer::s_enhance_model_flags);
	auto& spot_transform = spot_entity.GetComponent<TransformComponent>();
	spot_transform.scale = glm::vec3(0.1f, 0.1f, 0.1f);
	spot_transform.translation = glm::vec3(-37.f, 13.6f, -29.7f),
	spot_transform.rotation = glm::vec3(90.f, 0.f, 0.f);

	props.light_pos = glm::vec3(-37.f, 13.6f, -29.7f);
	props.ambient_color = glm::vec3(0.f, 0.f, 0.f);
	props.diffuse_color = glm::vec3(1.f, 0.8f, 0.8f);
	props.specular_color = glm::vec3(1.f, 1.f, 0.8f);
	props.direction = glm::vec3(0.79f, -0.5f, 0.35f);
	props.light_power = 30.f;
	props.max_distance = 100.f;
	props.theta_angle = 5.f;
	props.outer_cutoff = 7.f;
	props.cast_shadows = false;
	props.number_of_shadow_cascades = 1;
	spot_entity.AddComponent<SpotLightComponent>(props);

	props.light_pos = glm::vec3(-37.f, 13.6f, -29.7f);
	props.ambient_color = glm::vec3(0.f, 0.f, 0.f);
	props.diffuse_color = glm::vec3(1.f, 0.8f, 0.6f);
	props.specular_color = glm::vec3(1.f, 1.f, 0.6f);
	props.direction = glm::vec3(0.79f, -0.5f, 0.35f);
	props.light_power = 80.f;
	props.max_distance = 13.f;
	props.cast_shadows = false;
	props.number_of_shadow_cascades = 1;
	spot_entity.AddComponent<PointLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto nuke_entity = m_scene.CreateEntity("Nuke");
	nuke_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Nuke/Nuke.obj", ExampleLayer::s_enhance_model_flags);
	auto& nuke_transfrom = nuke_entity.GetComponent<TransformComponent>();
	nuke_transfrom.scale = glm::vec3(2.f, 2.f, 2.f);
	nuke_transfrom.translation = glm::vec3(-50.f, 18.f, -120.f);
	nuke_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto kratos_model = m_scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj", ExampleLayer::s_enhance_model_flags);
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(0.f, -2.8f, -18.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto miranda_entity = m_scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", ExampleLayer::s_enhance_model_flags);
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-12.f, -2.8f, -18.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RotationModelScript>(miranda_entity, 0.1f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto tank_entity = m_scene.CreateEntity("Tank");
	tank_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/WoT_LTP/LTP.obj", ExampleLayer::s_enhance_model_flags);
	auto& tank_transform = tank_entity.GetComponent<TransformComponent>();
	tank_transform.scale = glm::vec3(3.f, 3.f, 3.f);
	tank_transform.translation = glm::vec3(-25.f, -2.8f, -23.f);
	tank_transform.rotation = glm::vec3(0.f, 90.f, 0.f);
	auto& tank_scripts = tank_entity.AddComponent<NativeScriptComponent>();
	tank_scripts.AddScript<TankMovementScript>(tank_entity, glm::vec3(1.0f, 0.f, 0.0f), 50.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	std::vector<std::string> faces
	{
		"/skyboxes/Night/right.jpg",
		"/skyboxes/Night/left.jpg",
		"/skyboxes/Night/top.jpg",
		"/skyboxes/Night/bottom.jpg",
		"/skyboxes/Night/front.jpg",
		"/skyboxes/Night/back.jpg"
	};

	for (auto& face : faces)
	{
		face = WorkingDirectory::GetAssetsDirectory().string() + face;
	}

	auto cubemap_entity = m_scene.CreateEntity("Cubemap");
	cubemap_entity.AddComponent<SkyboxComponent>(faces, 0.f);
	auto& cubemap_transform = cubemap_entity.GetComponent<TransformComponent>();
	cubemap_transform.scale = glm::vec3(1.f, 1.f, 1.f);
	cubemap_transform.translation = glm::vec3(0.f, 0.f, 0.f);
	cubemap_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
}

void ExampleLayer::PrepareAnorLondoScene()
{
	auto camera_entity = m_scene.CreateEntity("Main Camera");
	auto& camera_comp =camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, Application::GetInstance()->GetWindow().GetAspectRatio(), 0.1f, 10000.f));
	auto camera = camera_comp.camera.get();
	camera->SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera->RotateCamera(-90.f, 0.f);
	camera->SetMovementSpeed(15.f);
	camera->SetSensivitity(5.f);
	camera->SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	//camera_scripts.AddScript<LogPositionScript>(camera_entity);

	auto light_entity = m_scene.CreateEntity("Main Light");
	Daedalus::LightProps props;
	props.light_pos = glm::vec3(0.3f, 1.f, 1.2f);
	props.ambient_color = glm::vec3(0.2f, 0.2f, 0.2f);
	props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
	props.specular_color = glm::vec3(0.95f, 0.85f, 0.65f);
	props.direction = glm::vec3(0.3f, 1.f, 1.2f);
	props.light_power = 1.f;
	props.cast_shadows = true;
	props.number_of_shadow_cascades = 6;
	light_entity.AddComponent<DirectionalLightComponent>(props);

	TransformComponent transform;
	transform.scale = glm::vec3(2.f, 2.f, 2.f);
	transform.translation = glm::vec3(510.f, -240.f, 730.f);
	transform.rotation = glm::vec3(0.f, -90.f, 0.f);
	CreateEntitiesForOBJFiles(WorkingDirectory::GetAssetsDirectory().string() + "/models/Anor Londo/", m_scene, transform);

	auto kratos_model = m_scene.CreateEntity("Kratos");
	kratos_model.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Kratos/Kratos.obj", ExampleLayer::s_enhance_model_flags);
	auto& kratos_transform = kratos_model.GetComponent<TransformComponent>();
	kratos_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	kratos_transform.translation = glm::vec3(9.f, -6.f, -20.f);
	kratos_transform.rotation = glm::vec3(0.f, 0.f, 0.f);

	auto miranda_entity = m_scene.CreateEntity("Miranda");
	miranda_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", ExampleLayer::s_enhance_model_flags);
	auto& miranda_transform = miranda_entity.GetComponent<TransformComponent>();
	miranda_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miranda_transform.translation = glm::vec3(-9.f, -6.f, -20.f);
	miranda_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& miranda_scripts = miranda_entity.AddComponent<NativeScriptComponent>();
	miranda_scripts.AddScript<RotationModelScript>(miranda_entity, 0.1f);

	auto marci_entity = m_scene.CreateEntity("Marci");
	marci_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Marci/marci.fbx", ExampleLayer::s_enhance_model_flags);
	auto& marci_transform = marci_entity.GetComponent<TransformComponent>();
	marci_transform.scale = glm::vec3(0.0002f, 0.0002f, 0.0002f);
	marci_transform.translation = glm::vec3(-9.f, -6.f, -35.f);
	marci_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	auto& marci_scripts = marci_entity.AddComponent<NativeScriptComponent>();
	marci_scripts.AddScript<RotationModelScript>(marci_entity, 0.1f);
}

void ExampleLayer::PreparePBRScene()
{
	Daedalus::LightProps props;

	auto camera_entity = m_scene.CreateEntity("Main Camera");

	const auto aspect_ratio = Application::GetInstance()->GetWindow().GetAspectRatio();
	auto& camera_comp = camera_entity.AddComponent<CameraComponent>(CameraProjectionProps(80.f, aspect_ratio, 0.1f, 200.f));

	//const auto scale_ortho = 20.f;
	//auto& camera_comp = camera_entity.AddComponent<CameraComponent>(-scale_ortho * aspect_ratio, scale_ortho * aspect_ratio, -scale_ortho, scale_ortho, -scale_ortho * 10.0f, scale_ortho * 10.f);

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto camera = camera_comp.camera.get();
	camera->SetPosition(glm::vec3(0.f, 0.f, 0.f));
	camera->SetPersonParams(7.f, 4.f);
	camera->RotateCamera(-90.f, 0.f);
	camera->SetMovementSpeed(15.f);
	camera->SetSensivitity(5.f);
	camera->SetZoomSpeed(1.f);
	auto& camera_scripts = camera_entity.AddComponent<NativeScriptComponent>();
	camera_scripts.AddScript<CameraController>(camera_entity);
	//camera_scripts.AddScript<LogPositionScript>(camera_entity);
	camera_scripts.AddScript<PlayerControllerScript>(camera_entity);

	camera_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Miranda/ME3_360_CHARACTER_Miranda_Lawson.obj", s_enhance_model_flags);
	auto& miran_transform = camera_entity.GetComponent<TransformComponent>();
	miran_transform.scale = glm::vec3(2.f, 2.f, 2.f);
	miran_transform.translation = glm::vec3(0.f, 0.f, 0.f);
	miran_transform.rotation = glm::vec3(0.f, 180.f, 0.f);

	props.light_pos = glm::vec3(0.f, 0.0f, 0.0f);
	props.ambient_color = glm::vec3(0.f, 0.f, 0.f);
	props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
	props.specular_color = glm::vec3(1.f, 1.f, 1.f);
	props.direction = glm::vec3{ 0.f, 0.f, 1.f };
	props.light_power = 4.f;
	props.max_distance = 100.f;
	props.theta_angle = 10.f;
	props.outer_cutoff = 35.f;
	props.cast_shadows = false;
	props.number_of_shadow_cascades = 1;
	camera_entity.AddComponent<SpotLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */
	 auto light_entity = m_scene.CreateEntity("Main Light");
	 props.light_pos = glm::vec3(0.78f, 1.0f, 0.6f);
	 props.ambient_color = glm::vec3(0.2f, 0.2f, 0.2f);
	 props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
	 props.specular_color = glm::vec3(1.f, 1.f, 1.f);
	 props.direction = glm::vec3(0.78f, 1.0f, 0.6f);
	 props.light_power = 1.0f;
	 props.cast_shadows = false;
	 props.number_of_shadow_cascades = 4;
	 light_entity.AddComponent<DirectionalLightComponent>(props);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	auto porsche_entity = m_scene.CreateEntity("Porsche");
	porsche_entity.AddComponent<RenderableObjectComponent>(WorkingDirectory::GetAssetsDirectory() / "models/Porsche/911.blend", ExampleLayer::s_enhance_model_flags);
	auto& porsche_transfrom = porsche_entity.GetComponent<TransformComponent>();
	porsche_transfrom.scale = glm::vec3(1.f, 1.f, 1.f);
	porsche_transfrom.translation = glm::vec3(0.f, 0.f, 0.f);
	porsche_transfrom.rotation = glm::vec3(-90.f, 0.f, 0.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	std::vector<std::string> faces
	{
		"/skyboxes/Sunny/right.jpg",
		"/skyboxes/Sunny/left.jpg",
		"/skyboxes/Sunny/top.jpg",
		"/skyboxes/Sunny/bottom.jpg",
		"/skyboxes/Sunny/front.jpg",
		"/skyboxes/Sunny/back.jpg"
	};

	for (auto& face : faces)
	{
		face = WorkingDirectory::GetAssetsDirectory().string() + face;
	}

	auto cubemap_entity = m_scene.CreateEntity("Cubemap");
	cubemap_entity.AddComponent<SkyboxComponent>(faces, 0.f);
	auto& cubemap_transform = cubemap_entity.GetComponent<TransformComponent>();
	cubemap_transform.scale = glm::vec3(1.f, 1.f, 1.f);
	cubemap_transform.translation = glm::vec3(0.f, 0.f, 0.f);
	cubemap_transform.rotation = glm::vec3(0.f, 0.f, 0.f);
	/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
}