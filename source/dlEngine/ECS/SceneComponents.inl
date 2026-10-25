template<> inline void Scene::OnComponentAdded<IDComponent>(IDComponent& component) {}
template<> inline void Scene::OnComponentRemoved<IDComponent>(IDComponent& component) {}

template<> inline void Scene::OnComponentAdded<TagComponent>(TagComponent& component) {}
template<> inline void Scene::OnComponentRemoved<TagComponent>(TagComponent& component) {}

template<> inline void Scene::OnComponentAdded<TransformComponent>(TransformComponent& component) {}
template<> inline void Scene::OnComponentRemoved<TransformComponent>(TransformComponent& component) {}

template<> inline void Scene::OnComponentAdded<RenderableObjectComponent>(RenderableObjectComponent& component) {}
template<> inline void Scene::OnComponentRemoved<RenderableObjectComponent>(RenderableObjectComponent& component) {}

template<> inline void Scene::OnComponentAdded<SkyboxComponent>(SkyboxComponent& component) {}
template<> inline void Scene::OnComponentRemoved<SkyboxComponent>(SkyboxComponent& component) {}

template<> inline void Scene::OnComponentAdded<CameraComponent>(CameraComponent& component) {}
template<> inline void Scene::OnComponentRemoved<CameraComponent>(CameraComponent& component) {}

template<> inline void Scene::OnComponentAdded<RigidBodyComponent>(RigidBodyComponent& component) {}
template<> inline void Scene::OnComponentRemoved<RigidBodyComponent>(RigidBodyComponent& component) {}

template<> inline void Scene::OnComponentAdded<KinematicBodyComponent>(KinematicBodyComponent& component) {}
template<> inline void Scene::OnComponentRemoved<KinematicBodyComponent>(KinematicBodyComponent& component) {}

template<> inline void Scene::OnComponentAdded<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> inline void Scene::OnComponentRemoved<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> inline void Scene::OnComponentAdded<PointLightComponent>(PointLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> inline void Scene::OnComponentRemoved<PointLightComponent>(PointLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> inline void Scene::OnComponentAdded<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system.AddLight(&component.light);
}
template<> inline void Scene::OnComponentRemoved<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system.RemoveLight(&component.light);
}

template<> inline void Scene::OnComponentAdded<NativeScriptComponent>(NativeScriptComponent& component) {}
template<> inline void Scene::OnComponentRemoved<NativeScriptComponent>(NativeScriptComponent& component) {}