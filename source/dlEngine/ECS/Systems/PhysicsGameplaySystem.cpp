#include "dlpch.h"
#include "PhysicsSystem.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/Scene.h"
#include "Core/Application.h"
#include "Events/PhysicsEvent.h"
#include "Events/EventsEngine.h"
#include "Debug/Log.h"
#include <algorithm>
#include <charconv>

using namespace Daedalus;

void PhysicsSystem::PrepareCharacters() const
{
	for (auto entity : m_registry.view<CharacterControllerComponent>())
	{
		auto& character = m_registry.get<CharacterControllerComponent>(entity);
		if (m_registry.any_of<RigidBodyComponent, KinematicBodyComponent>(entity))
		{
			character.Release();
			continue;
		}
		const auto id = m_registry.get<IDComponent>(entity).ID;
		if (character.handle && !m_world->HasCharacter(character.handle)) character.Release();
		if (!m_scene->HasUnitScaleAncestors(id))
		{
			glm::vec3 position = glm::vec3((*m_scene->GetWorldTransform(id))[3]);
			(void)m_world->GetCharacterPosition(character.handle, position);
			const auto orientation = m_scene->GetWorldOrientation(id);
			Log::Write(Log::Levels::Warn, Log::Categories::ECS, "Detaching character from scaled ancestors");
			(void)m_scene->SetParent(id);
			(void)m_scene->SetWorldPose(id, position, orientation);
			m_scene->UpdateWorldTransforms();
		}
		if (!character.handle)
		{
			const auto handle = m_world->CreateCharacter(character.description, glm::vec3((*m_scene->GetWorldTransform(id))[3]));
			character.Own(handle, m_world->MakeCharacterCleanup(handle));
			character.pose_initialized = false;
		}
		if (character.handle && !character.pose_initialized)
		{
			(void)m_world->WarpCharacter(character.handle, glm::vec3((*m_scene->GetWorldTransform(id))[3]));
			character.pose_initialized = true;
		}
		(void)m_world->SetCharacterVelocity(character.handle, character.velocity);
		if (character.jump_requested)
		{
			(void)m_world->JumpCharacter(character.handle);
			character.jump_requested = false;
		}
	}
}

bool PhysicsSystem::PrepareJoints(std::span<const UUID> owners) const
{
	const auto body_handle = [&](entt::entity entity) -> RigidBodyHandle
	{
		if (const auto* body = m_registry.try_get<RigidBodyComponent>(entity)) return body->handle;
		if (const auto* body = m_registry.try_get<KinematicBodyComponent>(entity)) return body->handle;
		return 0;
	};
	bool valid = true;
	for (auto entity : m_registry.view<JointComponent>())
	{
		if (!owners.empty() && std::find(owners.begin(), owners.end(), m_registry.get<IDComponent>(entity).ID) == owners.end()) continue;
		auto& component = m_registry.get<JointComponent>(entity);
		component.joints.resize(component.description.size());
		const auto first = body_handle(entity);
		for (size_t i = 0; i < component.description.size(); ++i)
		{
			const auto& record = component.description[i];
			uint64_t target = 0;
			const auto parsed = std::from_chars(record.target.data(), record.target.data() + record.target.size(), target);
			const auto found = m_scene->m_entity_map.find(UUID(target));
			const auto second = parsed.ec == std::errc() && parsed.ptr == record.target.data() + record.target.size() &&
				found != m_scene->m_entity_map.end() ? body_handle(found->second) : 0;
			auto& joint = component.joints[i];
			if (joint.handle && !m_world->HasJoint(joint.handle)) joint.Release();
			if (joint.first != first || joint.second != second || !first || !second)
			{
				joint.Release();
				joint.first = first;
				joint.second = second;
			}
			if (!joint.handle && first && second)
			{
				const auto handle = m_world->CreateJoint(first, second, record);
				joint.Own(handle, m_world->MakeJointCleanup(handle));
			}
			valid = valid && joint.handle != 0;
		}
	}
	return valid;
}

void PhysicsSystem::PublishEvents() const
{
	const auto callback = m_scene->m_physics_callback;
	const auto source = static_cast<uint64_t>(m_scene->m_physics_event_source);
	for (const auto& contact : m_world->GetContactEvents())
	{
		const auto first = m_body_entities.find(contact.first);
		const auto second = m_body_entities.find(contact.second);
		if (first == m_body_entities.end() || second == m_body_entities.end()) continue;
		ScenePhysicsEvent event(source, first->second, second->second, contact);
		if (auto* app = Application::GetInstance())
			(void)app->GetEventEngine().AddEvent(event.Clone());
		if (callback) (*callback)(event);
	}
	const auto events = m_world->GetContactEvents();
	std::erase_if(m_body_entities, [&](const auto& entry)
	{
		if (m_world->HasRigidBody(entry.first) || m_world->HasCharacter(entry.first)) return false;
		return std::none_of(events.begin(), events.end(), [&](const auto& event)
		{
			return event.phase != ContactPhase::Exit && (event.first == entry.first || event.second == entry.first);
		});
	});
}
