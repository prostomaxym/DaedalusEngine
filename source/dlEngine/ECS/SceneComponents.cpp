#include "dlpch.h"
#include "Scene.h"
#include "Components.h"
#include "NativeScriptComponent.h"
#include "Systems/RenderSystem.h"

using namespace Daedalus;

template<> void Scene::OnComponentAdded<IDComponent>(IDComponent&) {}
template<> void Scene::OnComponentRemoved<IDComponent>(IDComponent&) {}
template<> void Scene::OnComponentAdded<TagComponent>(TagComponent&) {}
template<> void Scene::OnComponentRemoved<TagComponent>(TagComponent&) {}
template<> void Scene::OnComponentAdded<TransformComponent>(TransformComponent&) {}
template<> void Scene::OnComponentRemoved<TransformComponent>(TransformComponent&) {}
template<> void Scene::OnComponentAdded<RenderableObjectComponent>(RenderableObjectComponent&) {}
template<> void Scene::OnComponentRemoved<RenderableObjectComponent>(RenderableObjectComponent&) {}
template<> void Scene::OnComponentAdded<SkyboxComponent>(SkyboxComponent&) {}
template<> void Scene::OnComponentRemoved<SkyboxComponent>(SkyboxComponent&) {}
template<> void Scene::OnComponentAdded<CameraComponent>(CameraComponent&) {}
template<> void Scene::OnComponentRemoved<CameraComponent>(CameraComponent&) {}
template<> void Scene::OnComponentAdded<RigidBodyComponent>(RigidBodyComponent& component) { component.Attach(GetPhysicsWorld()); }
template<> void Scene::OnComponentRemoved<RigidBodyComponent>(RigidBodyComponent&) {}
template<> void Scene::OnComponentAdded<KinematicBodyComponent>(KinematicBodyComponent& component) { component.Attach(GetPhysicsWorld()); }
template<> void Scene::OnComponentRemoved<KinematicBodyComponent>(KinematicBodyComponent&) {}
template<> void Scene::OnComponentAdded<NativeScriptComponent>(NativeScriptComponent&) {}
template<> void Scene::OnComponentRemoved<NativeScriptComponent>(NativeScriptComponent&) {}

template<> void Scene::OnComponentAdded<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system->AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<DirectionalLightComponent>(DirectionalLightComponent& component)
{
	m_render_system->RemoveLight(&component.light);
}
template<> void Scene::OnComponentAdded<PointLightComponent>(PointLightComponent& component)
{
	m_render_system->AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<PointLightComponent>(PointLightComponent& component)
{
	m_render_system->RemoveLight(&component.light);
}
template<> void Scene::OnComponentAdded<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system->AddLight(&component.light);
}
template<> void Scene::OnComponentRemoved<SpotLightComponent>(SpotLightComponent& component)
{
	m_render_system->RemoveLight(&component.light);
}
