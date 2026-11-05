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
		const auto handle = m_entity_map.at(child);
		if (m_registry.any_of<RigidBodyComponent, KinematicBodyComponent>(handle)) return false;
		for (auto ancestor = parent; static_cast<uint64_t>(ancestor) != 0; ancestor = m_transform_nodes.at(ancestor).parent)
			if (ancestor == child) return false;
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
		auto& node = m_transform_nodes.at(id);
		const auto* transform = m_registry.try_get<TransformComponent>(m_entity_map.at(id));
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
			++updated;
		}
		node.dirty = false;
	}
	return updated;
}
