template<> void Scene::OnComponentAdded<IDComponent>(IDComponent& component) {}
template<> void Scene::OnComponentRemoved<IDComponent>(IDComponent& component) {}

template<> void Scene::OnComponentAdded<TagComponent>(TagComponent& component) {}
template<> void Scene::OnComponentRemoved<TagComponent>(TagComponent& component) {}

template<> void Scene::OnComponentAdded<TransformComponent>(TransformComponent& component) {}
template<> void Scene::OnComponentRemoved<TransformComponent>(TransformComponent& component) {}

template<> void Scene::OnComponentAdded<RenderableObjectComponent>(RenderableObjectComponent& component) {}
template<> void Scene::OnComponentRemoved<RenderableObjectComponent>(RenderableObjectComponent& component) {}

template<> void Scene::OnComponentAdded<SkyboxComponent>(SkyboxComponent& component) {}
template<> void Scene::OnComponentRemoved<SkyboxComponent>(SkyboxComponent& component) {}

template<> void Scene::OnComponentAdded<CameraComponent>(CameraComponent& component) {}
template<> void Scene::OnComponentRemoved<CameraComponent>(CameraComponent& component) {}

template<> void Scene::OnComponentAdded<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> void Scene::OnComponentAdded<PointLightComponent>(PointLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<PointLightComponent>(PointLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> void Scene::OnComponentAdded<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> void Scene::OnComponentAdded<NativeScriptComponent>(NativeScriptComponent& component) {}
template<> void Scene::OnComponentRemoved<NativeScriptComponent>(NativeScriptComponent& component) {}