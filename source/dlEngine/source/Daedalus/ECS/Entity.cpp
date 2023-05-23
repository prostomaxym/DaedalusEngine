#include "dlpch.h"

#include "Daedalus/ECS/Entity.h"

using namespace Daedalus;

Entity::Entity(entt::entity handle, Scene* scene)
	: m_entity_handle(handle), m_scene(scene)
{
}