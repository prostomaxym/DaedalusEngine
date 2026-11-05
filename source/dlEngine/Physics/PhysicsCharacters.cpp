#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include <cmath>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

bool PhysicsWorld::ValidateCharacterSettings(const CharacterSettings& settings)
{
	return std::isfinite(settings.radius) && settings.radius > 0.f &&
		std::isfinite(settings.height) && settings.height > 0.f &&
		std::isfinite(settings.step_height) && settings.step_height >= 0.f &&
		std::isfinite(settings.max_slope) && settings.max_slope >= 0.f && settings.max_slope < 90.f &&
		std::isfinite(settings.jump_speed) && settings.jump_speed > 0.f &&
		std::isfinite(settings.fall_speed) && settings.fall_speed > 0.f &&
		Finite(settings.gravity) && settings.gravity.x == 0.f && settings.gravity.z == 0.f && settings.gravity.y <= 0.f &&
		settings.group > 0 && settings.group <= 65535 && settings.mask <= 65535;
}

CharacterHandle PhysicsWorld::CreateCharacter(const CharacterSettings& settings, const glm::vec3& position)
{
	m_impl->CheckThread();
	if (!ValidateCharacterSettings(settings) || !Finite(position)) return 0;
	Impl::Character character;
	character.m_shape = std::make_unique<btCapsuleShape>(settings.radius, settings.height);
	character.m_ghost = std::make_unique<btPairCachingGhostObject>();
	character.m_ghost->setWorldTransform(Pose(position, glm::quat(1.f, 0.f, 0.f, 0.f)));
	character.m_ghost->setCollisionShape(character.m_shape.get());
	character.m_ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
	character.m_controller = std::make_unique<btKinematicCharacterController>(character.m_ghost.get(), character.m_shape.get(),
		settings.step_height, btVector3(0.f, 1.f, 0.f));
	character.m_controller->setGravity(ToBullet(settings.gravity));
	character.m_controller->setMaxSlope(glm::radians(settings.max_slope));
	character.m_controller->setJumpSpeed(settings.jump_speed);
	character.m_controller->setFallSpeed(settings.fall_speed);
	// Bullet's constructor rotates its Z-up frame; our capsule is already Y-aligned.
	character.m_controller->warp(ToBullet(position));
	const auto handle = NextHandle();
	m_impl->m_world.addCollisionObject(character.m_ghost.get(), static_cast<int>(settings.group), static_cast<int>(settings.mask));
	m_impl->m_world.addAction(character.m_controller.get());
	m_impl->m_handles.emplace(character.m_ghost.get(), handle);
	m_impl->m_characters.emplace(handle, std::move(character));
	return handle;
}

void PhysicsWorld::Impl::RemoveCharacter(CharacterHandle handle)
{
	CheckThread();
	const auto it = m_characters.find(handle);
	if (it == m_characters.end()) return;
	m_world.removeAction(it->second.m_controller.get());
	m_world.removeCollisionObject(it->second.m_ghost.get());
	m_handles.erase(it->second.m_ghost.get());
	m_characters.erase(it);
}

void PhysicsWorld::RemoveCharacter(CharacterHandle handle) { m_impl->RemoveCharacter(handle); }
bool PhysicsWorld::HasCharacter(CharacterHandle handle) const
{
	m_impl->CheckThread();
	return m_impl->m_characters.contains(handle);
}

bool PhysicsWorld::SetCharacterVelocity(CharacterHandle handle, const glm::vec3& velocity)
{
	m_impl->CheckThread();
	const auto it = m_impl->m_characters.find(handle);
	if (it == m_impl->m_characters.end() || !Finite(velocity) || velocity.y != 0.f) return false;
	it->second.m_velocity = velocity;
	return true;
}

bool PhysicsWorld::GetCharacterPosition(CharacterHandle handle, glm::vec3& position) const
{
	m_impl->CheckThread();
	const auto it = m_impl->m_characters.find(handle);
	if (it == m_impl->m_characters.end()) return false;
	position = FromBullet(it->second.m_ghost->getWorldTransform().getOrigin());
	return true;
}

bool PhysicsWorld::WarpCharacter(CharacterHandle handle, const glm::vec3& position)
{
	m_impl->CheckThread();
	const auto it = m_impl->m_characters.find(handle);
	if (it == m_impl->m_characters.end() || !Finite(position)) return false;
	it->second.m_controller->reset(&m_impl->m_world);
	it->second.m_controller->warp(ToBullet(position));
	it->second.m_has_stepped = false;
	m_impl->m_world.updateSingleAabb(it->second.m_ghost.get());
	return true;
}

bool PhysicsWorld::IsCharacterGrounded(CharacterHandle handle) const
{
	m_impl->CheckThread();
	const auto it = m_impl->m_characters.find(handle);
	return it != m_impl->m_characters.end() && it->second.m_has_stepped && it->second.m_controller->onGround();
}

bool PhysicsWorld::JumpCharacter(CharacterHandle handle)
{
	m_impl->CheckThread();
	const auto it = m_impl->m_characters.find(handle);
	if (it == m_impl->m_characters.end() || !it->second.m_has_stepped || !it->second.m_controller->canJump()) return false;
	it->second.m_controller->jump();
	return true;
}

std::function<void()> PhysicsWorld::MakeCharacterCleanup(CharacterHandle handle) const
{
	std::weak_ptr<Impl> owner = m_impl;
	return [owner, handle] { if (auto world = owner.lock()) world->RemoveCharacter(handle); };
}
