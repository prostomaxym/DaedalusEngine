#include "dlpch.h"
#include "RenderSystem.h"

#include "Daedalus/ECS/Components.h"
#include "Daedalus/ECS/Entity.h"
#include "Daedalus/ECS/Scene.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

void RenderSystem::OnStartRuntime() const
{
	std::map<uint32_t, LightSource*> lights;

	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light = entity.GetComponent<DirectionalLightComponent>().light;
		const auto id = entity.GetUUID();
		lights[id] = &light;
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<PointLightComponent>();
		auto& light = light_component.light;
		const auto id = entity.GetUUID();
		lights[id] = &light;
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<SpotLightComponent>();
		auto& light = light_component.light;
		const auto id = entity.GetUUID();
		lights[id] = &light;
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

void RenderSystem::RenderSkybox() const
{
	const auto cubemap_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::CubemapShader);
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

		Renderer::Submit(&model_component.model, transform_component);
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
		spheres.push_back(model_component.model.GetBoundingSphere());
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
		aabbs.push_back(model_component.model.GetBoundingAABB());
	}

	return AABB::CalculateCommonBoundingAABB(aabbs);
}