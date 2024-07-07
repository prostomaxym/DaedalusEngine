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
	RenderCommand::SetClearColor({ 1.0f, 1.0f, 1.0f, 1.0 });
	Renderer::BeginScene(*m_camera);

	UpdateStaticLighting();
	DoStaticLightPass();

	Renderer::EndScene();
}

void RenderSystem::OnUpdateRuntime(DeltaTime dt) const
{
	RenderCommand::SetClearColor({ 1.0f, 1.0f, 1.0f, 1.0 });
	Renderer::BeginScene(*m_camera);

	UpdateDynamicLighting();
	DoDynamicLightPass();
	DoColorPass();

	Renderer::EndScene();
}

void RenderSystem::DoStaticLightPass() const
{
	Renderer::UpdateStaticNumberOfShadowCasters(CountStaticShadowCasters());

	const auto shadow_fb = Renderer::GetStaticShadowFramebuffer();
	shadow_fb->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
	RenderCommand::SetViewport(0, 0, RenderCommand::GetMaxTextureSize(), RenderCommand::GetMaxTextureSize());

	const auto shadow_shader = Renderer::s_shader_library->Get(ShaderConstants::ShadowShader);

	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (!light_component.light.CastShadow() || light_component.is_dynamic)
			continue;

		const auto models_view = m_registry.view<RenderableObjectComponent>();
		for (const auto e : models_view)
		{
			Entity entity = { e, m_scene };

			const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
			const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

			Renderer::SubmitForShadowBuffer(shadow_shader.get(), &model_component.model, transform_component);
		}
	}

	shadow_fb->Unbind();
}

void RenderSystem::DoDynamicLightPass() const
{
	Renderer::UpdateDynamicNumberOfShadowCasters(CountDynamicShadowCasters());

	const auto shadow_fb = Renderer::GetDynamicShadowFramebuffer();
	shadow_fb->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
	RenderCommand::SetViewport(0, 0, GraphicsConfig::GetShadowBufferWidth(), GraphicsConfig::GetShadowBufferHeight());

	const auto shadow_shader = Renderer::s_shader_library->Get(ShaderConstants::ShadowShader);
	
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (!light_component.light.CastShadow() || !light_component.is_dynamic)
			continue;

		const auto models_view = m_registry.view<RenderableObjectComponent>();
		for (const auto e : models_view)
		{
			Entity entity = { e, m_scene };

			const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
			const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

			Renderer::SubmitForShadowBuffer(shadow_shader.get(), &model_component.model, transform_component);
		}
	}

	shadow_fb->Unbind();
}

void RenderSystem::DoColorPass() const
{
	const auto standard_shader = Renderer::s_shader_library->Get(ShaderConstants::StandardShader);
	Renderer::BindShadowMap(standard_shader.get());
	RenderCommand::SetViewport(0, 0, m_viewport_width, m_viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);

	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::Submit(model_component.shader.get(), &model_component.model, transform_component);
	}

	const auto cubemap_view = m_registry.view<CubemapComponent>();
	for (const auto e : cubemap_view)
	{
		Entity entity = { e, m_scene };

		const auto& cubemap_component = entity.GetComponent<CubemapComponent>();
		Renderer::Submit(cubemap_component.shader.get(), &cubemap_component.cubemap, m_camera->GetProjectionViewMatrixWithoutTranslation(cubemap_component.rotation_angle));
	}
}

int RenderSystem::CountStaticShadowCasters() const
{
	int counter = 0;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.light.CastShadow() && !light_component.is_dynamic)
			counter++;
	}

	return counter;
}

int RenderSystem::CountDynamicShadowCasters() const
{
	int counter = 0;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.light.CastShadow() && light_component.is_dynamic)
			counter++;
	}

	return counter;
}

BoundingSphere RenderSystem::CaltulateSceneBoundingSphere() const
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

void RenderSystem::UpdateStaticLighting() const
{
	const auto scene_sphere = CaltulateSceneBoundingSphere();

	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_space_matrices_SSBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	auto i = 0;
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();
		auto& light = light_component.light;

		if (light_component.is_dynamic)
			continue;

		light.UpdateSSBOForSceneFrustum(scene_sphere);

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(i);
			light_space_matrices_SSBOs.push_back(light.GetLightSpaceMatrix());
			i++;
		}

		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };

		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateStaticLightSSBO(light_SSBOs);
}

void RenderSystem::UpdateDynamicLighting() const
{
	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_space_matrices_SSBOs;

	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	auto i = 0;
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();
		auto& light = light_component.light;

		if (!light_component.is_dynamic)
			continue;

		light.UpdateSSBOForViewFrustum(m_camera->GetProjectionMatrix(0.2f), m_camera->GetViewMatrix());

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(i);
			light_space_matrices_SSBOs.push_back(light.GetLightSpaceMatrix());
			i++;
		}
			
		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateLightSpaceMatricesSSBO(light_space_matrices_SSBOs);
	Renderer::UpdateDynamicLightSSBO(light_SSBOs);
}
