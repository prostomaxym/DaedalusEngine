#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Daedalus
{
	using RigidBodyHandle = uint64_t;
	using CharacterHandle = uint64_t;
	using JointHandle = uint64_t;

	struct ColliderDescription
	{
		std::string shape = "box";
		glm::vec3 half_extents{ 0.5f };
		float radius = 0.5f;
		// Capsule height excludes the hemispheres. All axial shapes use local +Y.
		float height = 1.f;
		std::vector<glm::vec3> vertices;
		std::vector<uint32_t> indices;
	};

	struct PhysicsBodySettings
	{
		uint32_t group = 1;
		uint32_t mask = 65535;
		bool trigger = false;
		float friction = 0.5f;
		float restitution = 0.f;
		float rolling_friction = 0.f;
		float linear_damping = 0.f;
		float angular_damping = 0.f;
	};

	struct PhysicsQueryFilter
	{
		uint32_t group = 1;
		uint32_t mask = 65535;
		RigidBodyHandle ignore = 0;
		bool include_triggers = false;
	};

	struct PhysicsQueryHit
	{
		RigidBodyHandle body = 0;
		glm::vec3 position{ 0.f };
		glm::vec3 normal{ 0.f };
		float fraction = 0.f;
	};

	enum class ContactPhase { Enter, Stay, Exit };

	struct PhysicsContact
	{
		RigidBodyHandle first = 0;
		RigidBodyHandle second = 0;
		ContactPhase phase = ContactPhase::Enter;
		bool trigger = false;
		glm::vec3 position{ 0.f };
		// Points from first toward second; one representative deepest contact per pair.
		glm::vec3 normal{ 0.f };
		float impulse = 0.f;
	};

	struct CharacterSettings
	{
		float radius = 0.5f;
		float height = 1.f;
		float step_height = 0.3f;
		float max_slope = 45.f;
		float jump_speed = 5.f;
		float fall_speed = 55.f;
		glm::vec3 gravity{ 0.f, -9.81f, 0.f };
		uint32_t group = 32;
		uint32_t mask = 65535;
	};

	struct JointSettings
	{
		std::string type = "fixed";
		glm::vec3 pivot_a{ 0.f };
		glm::vec3 pivot_b{ 0.f };
		// Local frame Euler angles in degrees. Hinge uses frame Z; slider uses frame X.
		glm::vec3 rotation_a{ 0.f };
		glm::vec3 rotation_b{ 0.f };
		float lower_limit = 0.f;
		float upper_limit = 0.f;
		float angular_lower_limit = 0.f;
		float angular_upper_limit = 0.f;
		glm::vec3 swing_limits{ 45.f, 45.f, 45.f };
		bool collide_connected = false;
		float breaking_impulse = 0.f;
	};
}
