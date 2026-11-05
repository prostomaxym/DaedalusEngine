#include "dlpch.h"
#include "Scene.h"
#include "Components.h"

#include <algorithm>

using namespace Daedalus;

bool Scene::SetParent(UUID child, UUID parent)
{
	auto child_it = m_transform_nodes.find(child);
	if (child_it == m_transform_nodes.end() || child == parent) return false;
	if (static_cast<uint64_t>(parent) != 0)
	{
		if (!m_transform_nodes.contains(parent)) return false;
		for (auto ancestor = parent; static_cast<uint64_t>(ancestor) != 0; ancestor = m_transform_nodes.at(ancestor).parent)
			if (ancestor == child) return false;
	}
	// Check the whole moved subtree: a visual ancestor may itself contain bodies.
	bool scaled = false;
	for (auto ancestor = parent; static_cast<uint64_t>(ancestor) != 0; ancestor = m_transform_nodes.at(ancestor).parent)
	{
		const auto* transform = m_registry.try_get<TransformComponent>(m_entity_map.at(ancestor));
		scaled = scaled || (transform && transform->scale != glm::vec3(1.f));
	}
	std::vector<std::pair<UUID, bool>> pending{ { child, scaled } };
	for (size_t index = 0; index < pending.size(); ++index)
	{
		const auto [id, inherited_scale] = pending[index];
		const auto handle = m_entity_map.at(id);
		if (static_cast<uint64_t>(parent) != 0 && inherited_scale && m_registry.any_of<RigidBodyComponent, KinematicBodyComponent, CharacterControllerComponent>(handle)) return false;
		const auto* transform = m_registry.try_get<TransformComponent>(handle);
		for (auto descendant : m_transform_nodes.at(id).children)
			pending.emplace_back(descendant, inherited_scale || (transform && transform->scale != glm::vec3(1.f)));
	}
	auto& node = child_it->second;
	if (node.parent == parent) return true;
	if (static_cast<uint64_t>(node.parent) != 0)
	{
		auto& siblings = m_transform_nodes.at(node.parent).children;
		std::erase(siblings, child);
	}
	node.parent = parent;
	if (static_cast<uint64_t>(parent) != 0) m_transform_nodes.at(parent).children.push_back(child);
	node.dirty = true;
	m_hierarchy_dirty = true;
	return true;
}

Daedalus::UUID Scene::GetParent(UUID child) const
{
	const auto it = m_transform_nodes.find(child);
	return it == m_transform_nodes.end() ? UUID(0) : it->second.parent;
}

std::span<const Daedalus::UUID> Scene::GetChildren(UUID parent) const
{
	const auto it = m_transform_nodes.find(parent);
	return it == m_transform_nodes.end() ? std::span<const UUID>{} : std::span<const UUID>(it->second.children);
}

const glm::mat4* Scene::GetWorldTransform(UUID entity) const
{
	const auto it = m_transform_nodes.find(entity);
	return it == m_transform_nodes.end() ? nullptr : &it->second.world;
}

glm::quat Scene::GetWorldOrientation(UUID entity) const
{
	const auto it = m_transform_nodes.find(entity);
	return it == m_transform_nodes.end() ? glm::quat(1.f, 0.f, 0.f, 0.f) : it->second.world_orientation;
}

bool Scene::SetWorldPose(UUID entity, const glm::vec3& position, const glm::quat& orientation)
{
	const auto it = m_entity_map.find(entity);
	if (it == m_entity_map.end()) return false;
	if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
		!std::isfinite(orientation.w) || !std::isfinite(orientation.x) || !std::isfinite(orientation.y) ||
		!std::isfinite(orientation.z) || glm::length(orientation) < 0.000001f) return false;
	auto* transform = m_registry.try_get<TransformComponent>(it->second);
	if (!transform) return false;
	const auto parent = GetParent(entity);
	if (static_cast<uint64_t>(parent) != 0)
	{
		const auto& matrix = *GetWorldTransform(parent);
		if (!std::isfinite(glm::determinant(matrix)) || glm::determinant(matrix) == 0.f) return false;
		transform->translation = glm::vec3(glm::inverse(matrix) * glm::vec4(position, 1.f));
		transform->SetOrientationQuat(glm::inverse(GetWorldOrientation(parent)) * glm::normalize(orientation));
	}
	else
	{
		transform->translation = position;
		transform->SetOrientationQuat(glm::normalize(orientation));
	}
	return true;
}

bool Scene::HasUnitScaleAncestors(UUID entity) const
{
	for (auto parent = GetParent(entity); static_cast<uint64_t>(parent) != 0; parent = GetParent(parent))
	{
		const auto* transform = m_registry.try_get<TransformComponent>(m_entity_map.at(parent));
		if (transform && transform->scale != glm::vec3(1.f)) return false;
	}
	return true;
}

bool Scene::SetEntityLink(UUID source, std::string name, UUID target)
{
	const auto it = m_transform_nodes.find(source);
	if (it == m_transform_nodes.end() || name.empty() || !m_transform_nodes.contains(target)) return false;
	it->second.links.insert_or_assign(std::move(name), target);
	return true;
}

Daedalus::UUID Scene::GetEntityLink(UUID source, std::string_view name) const
{
	const auto it = m_transform_nodes.find(source);
	if (it == m_transform_nodes.end()) return UUID(0);
	const auto link = it->second.links.find(name);
	return link == it->second.links.end() ? UUID(0) : link->second;
}

void Scene::RemoveFromHierarchy(UUID entity)
{
	auto& node = m_transform_nodes.at(entity);
	if (static_cast<uint64_t>(node.parent) != 0)
		std::erase(m_transform_nodes.at(node.parent).children, entity);
	for (auto child : node.children)
	{
		auto& child_node = m_transform_nodes.at(child);
		child_node.parent = UUID(0);
		child_node.dirty = true;
	}
	m_transform_nodes.erase(entity);
	for (auto& [id, remaining] : m_transform_nodes)
		std::erase_if(remaining.links, [entity](const auto& link) { return link.second == entity; });
	m_hierarchy_dirty = true;
}

size_t Scene::UpdateWorldTransforms()
{
	if (m_hierarchy_dirty)
	{
		m_transform_order.clear();
		for (const auto& [id, node] : m_transform_nodes)
			if (static_cast<uint64_t>(node.parent) == 0) m_transform_order.push_back(id);
		for (size_t index = 0; index < m_transform_order.size(); ++index)
		{
			const auto& children = m_transform_nodes.at(m_transform_order[index]).children;
			m_transform_order.insert(m_transform_order.end(), children.begin(), children.end());
		}
		m_hierarchy_dirty = false;
	}

	size_t updated = 0;
	for (auto id : m_transform_order)
	{
		updated += UpdateWorldTransform(id) ? 1u : 0u;
	}
	return updated;
}

bool Scene::UpdateWorldTransform(UUID id)
{
	auto& node = m_transform_nodes.at(id);
	const auto handle = m_entity_map.at(id);
	const auto* transform = m_registry.try_get<TransformComponent>(handle);
	const TransformComponent identity;
	const auto& local = transform ? *transform : identity;
	const bool local_changed = node.translation != local.translation || node.rotation != local.rotation || node.scale != local.scale;
	if (local_changed || node.dirty)
	{
		node.translation = local.translation;
		node.rotation = local.rotation;
		node.scale = local.scale;
		node.local = local.GetTransform();
	}
	const auto* parent = static_cast<uint64_t>(node.parent) != 0 ? &m_transform_nodes.at(node.parent) : nullptr;
	node.changed = local_changed || node.dirty || (parent && parent->changed);
	if (node.changed)
	{
		node.world = parent ? parent->world * node.local : node.local;
		node.world_orientation = glm::normalize(parent ? parent->world_orientation * local.GetOrientationQuat() : local.GetOrientationQuat());
	}
	node.dirty = false;
	const auto position = glm::vec3(node.world[3]);
	if (auto* camera = m_registry.try_get<CameraComponent>(handle); camera && camera->camera)
		camera->camera->SetPose(position, node.world_orientation, parent ? parent->world_orientation * glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, 1.f, 0.f));
	const auto direction = node.world_orientation * glm::vec3(0.f, 0.f, -1.f);
	auto update_light = [&](auto* component)
	{
		if (!component) return;
		component->light.SetPosition(position);
		component->light.SetDirection(direction);
	};
	update_light(m_registry.try_get<DirectionalLightComponent>(handle));
	update_light(m_registry.try_get<PointLightComponent>(handle));
	update_light(m_registry.try_get<SpotLightComponent>(handle));
	return node.changed;
}
