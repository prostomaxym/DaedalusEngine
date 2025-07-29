#include "dlpch.h"
#include "RenderSystem.h"

#include "Core/ResourceManager.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/Scene.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RenderConstants.h"

using namespace Daedalus;

void RenderSystem::OnStartRuntime() const
{
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	const auto point_view = m_registry.view<PointLightComponent>();
	const auto spot_view = m_registry.view<SpotLightComponent>();

	std::vector<LightSource*> lights;
	lights.reserve(dir_view.size() + point_view.size() + spot_view.size());

	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto light = &entity.GetComponent<DirectionalLightComponent>().light;
		lights.push_back(light);
	}

	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		auto light = &entity.GetComponent<PointLightComponent>().light;
		lights.push_back(light);
	}

	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		auto light = &entity.GetComponent<SpotLightComponent>().light;
		lights.push_back(light);
	}

	Renderer::SetLights(lights);
}

void RenderSystem::OnUpdateRuntime(DeltaTime dt) const
{
	Renderer::BeginFrame(m_camera, m_registry.view<RenderableObjectComponent>().size());
	SubmitModels();
	Renderer::FlushPipeline();
	RenderSkybox();
}

void RenderSystem::SetViewportSize(int width, int height)
{
	m_viewport_width = width;
	m_viewport_height = height;
	Renderer::OnWindowResize(width, height);
}

void RenderSystem::AddLight(LightSource* light) const
{
	Renderer::AddLight(light);
}

void RenderSystem::RemoveLight(LightSource* light) const
{
	Renderer::RemoveLight(light);
}

void RenderSystem::RenderSkybox() const
{
	const auto cubemap_shader = ResourceManager::GetShader(ShaderConstants::CubemapShader);
	cubemap_shader->Bind();
	const auto cubemap_view = m_registry.view<SkyboxComponent>();
	for (const auto e : cubemap_view)
	{
		Entity entity = { e, m_scene };

		const auto& cubemap_component = entity.GetComponent<SkyboxComponent>();
		Renderer::Draw(cubemap_shader.get(), &cubemap_component.cubemap,
			m_camera->GetProjectionViewMatrixWithoutTranslation(cubemap_component.rotation_angle));
	}
	cubemap_shader->Unbind();
}

void RenderSystem::SubmitModels() const
{
	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::Submit(model_component.model.get(), transform_component);
	}
}

BoundingSphere RenderSystem::CalculateSceneBoundingSphere() const
{
	const auto models_view = m_registry.view<RenderableObjectComponent>();

	std::vector<BoundingSphere> spheres;
	spheres.reserve(models_view.size());

	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		spheres.push_back(model_component.model->GetBoundingSphere());
	}

	return BoundingSphere::CalculateCommonBoundingSphere(spheres);
}

AABB RenderSystem::CalculateSceneBoundingAABB() const
{
	const auto models_view = m_registry.view<RenderableObjectComponent>();

	std::vector<AABB> aabbs;
	aabbs.reserve(models_view.size());

	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		aabbs.push_back(model_component.model->GetBoundingAABB());
	}

	return AABB::CalculateCommonBoundingAABB(aabbs);
}