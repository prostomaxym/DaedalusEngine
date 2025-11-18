#include "dlpch.h"
#include "Physics.h"
#include <algorithm>

#include <btBulletDynamicsCommon.h>

using namespace Daedalus;

std::unique_ptr<Physics::Impl> Physics::s_impl = nullptr;

// Helper conversions between handle and pointer
static RigidBodyHandle ToHandle(btRigidBody* body)
{
	return static_cast<RigidBodyHandle>(reinterpret_cast<uintptr_t>(body));
}

static btRigidBody* FromHandle(RigidBodyHandle handle)
{
	if (handle == 0) return nullptr;
	return reinterpret_cast<btRigidBody*>(static_cast<uintptr_t>(handle));
}

// Implementation (PImpl) that contains Bullet details.
struct Physics::Impl
{
	// Bullet core objects
	std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
	std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

	// Owned allocations for cleanup
	std::vector<btCollisionShape*> ownedShapes;
	std::vector<btMotionState*> ownedMotionStates;
	std::vector<btRigidBody*> ownedBodies;

	// Collision callback using opaque handles
	Physics::CollisionCallback collisionCallback;

	Impl() = default;
	~Impl() = default;

	bool Initialize(const glm::vec3& gravity)
	{
		if (dynamicsWorld)
			return true;

		collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
		dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
		broadphase = std::make_unique<btDbvtBroadphase>();
		solver = std::make_unique<btSequentialImpulseConstraintSolver>();

		dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
			dispatcher.get(),
			broadphase.get(),
			solver.get(),
			collisionConfig.get()
		);

		if (!dynamicsWorld)
			return false;

		dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
		return true;
	}

	void Shutdown()
	{
		if (!dynamicsWorld)
			return;

		// Remove bodies from world and delete them
		for (btRigidBody* body : ownedBodies)
		{
			if (!body) continue;
			dynamicsWorld->removeRigidBody(body);
			// motion states will be deleted below
			delete body;
		}
		ownedBodies.clear();

		// delete motion states
		for (btMotionState* ms : ownedMotionStates)
			delete ms;
		ownedMotionStates.clear();

		// delete shapes
		for (btCollisionShape* shape : ownedShapes)
			delete shape;
		ownedShapes.clear();

		// reset Bullet world parts
		dynamicsWorld.reset();
		solver.reset();
		broadphase.reset();
		dispatcher.reset();
		collisionConfig.reset();

		collisionCallback = nullptr;
	}

	RigidBodyHandle CreateBoxRigidBody(float mass, const glm::vec3& halfExtents, const glm::vec3& position, const glm::quat& orientation)
	{
		if (!dynamicsWorld) return 0;

		btBoxShape* box = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		ownedShapes.push_back(box);

		btVector3 localInertia(0, 0, 0);
		if (mass != 0.0f)
			box->calculateLocalInertia(mass, localInertia);

		btQuaternion rot(orientation.x, orientation.y, orientation.z, orientation.w);
		btTransform transform(rot, btVector3(position.x, position.y, position.z));

		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		ownedMotionStates.push_back(motionState);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, box, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);

		ownedBodies.push_back(body);
		return ToHandle(body);
	}

	RigidBodyHandle CreateSphereRigidBody(float mass, float radius, const glm::vec3& position, const glm::quat& orientation)
	{
		if (!dynamicsWorld) return 0;

		btSphereShape* sphere = new btSphereShape(radius);
		ownedShapes.push_back(sphere);

		btVector3 localInertia(0, 0, 0);
		if (mass != 0.0f)
			sphere->calculateLocalInertia(mass, localInertia);

		btQuaternion rot(orientation.x, orientation.y, orientation.z, orientation.w);
		btTransform transform(rot, btVector3(position.x, position.y, position.z));

		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		ownedMotionStates.push_back(motionState);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, sphere, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);

		ownedBodies.push_back(body);
		return ToHandle(body);
	}

	void RemoveRigidBody(RigidBodyHandle handle)
	{
		btRigidBody* body = FromHandle(handle);
		if (!body || !dynamicsWorld) return;

		dynamicsWorld->removeRigidBody(body);

		auto it = std::find(ownedBodies.begin(), ownedBodies.end(), body);
		if (it != ownedBodies.end())
			ownedBodies.erase(it);

		btMotionState* ms = body->getMotionState();
		delete body;
		if (ms)
		{
			auto itms = std::find(ownedMotionStates.begin(), ownedMotionStates.end(), ms);
			if (itms != ownedMotionStates.end())
				ownedMotionStates.erase(itms);
			delete ms;
		}

		// Note: shapes currently remain in ownedShapes and are freed on Shutdown.
	}

	void SetGravity(const glm::vec3& gravity)
	{
		if (dynamicsWorld)
			dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
	}

	bool GetRigidBodyTransform(RigidBodyHandle handle, glm::vec3& outPosition, glm::quat& outOrientation) const
	{
		btRigidBody* body = FromHandle(handle);
		if (!body) return false;

		btTransform t;
		if (body->getMotionState())
			body->getMotionState()->getWorldTransform(t);
		else
			t = body->getWorldTransform();

		const btVector3& o = t.getOrigin();
		const btQuaternion& q = t.getRotation();

		outPosition = glm::vec3(o.x(), o.y(), o.z());
		outOrientation = glm::quat(q.getW(), q.getX(), q.getY(), q.getZ());
		return true;
	}

	bool SetRigidBodyTransform(RigidBodyHandle handle, const glm::vec3& position, const glm::quat& orientation)
	{
		btRigidBody* body = FromHandle(handle);
		if (!body) return false;

		btTransform t(btQuaternion(orientation.x, orientation.y, orientation.z, orientation.w),
			btVector3(position.x, position.y, position.z));

		body->setWorldTransform(t);
		if (body->getMotionState())
			body->getMotionState()->setWorldTransform(t);

		// ensure body is awake
		body->activate();
		return true;
	}

	void RegisterCollisionCallback(Physics::CollisionCallback cb)
	{
		collisionCallback = std::move(cb);
	}

private:
	void ProcessCollisions()
	{
		if (!dynamicsWorld || !collisionCallback) return;

		btDispatcher* disp = dynamicsWorld->getDispatcher();
		int numManifolds = disp->getNumManifolds();

		// To avoid duplicate callbacks for the same pair, we'll use an unordered_set of ordered pair keys.
		struct PairHash
		{
			size_t operator()(const std::pair<const void*, const void*>& p) const noexcept
			{
				// simple pointer hashing combination
				return (std::hash<const void*>()(p.first) * 31) ^ std::hash<const void*>()(p.second);
			}
		};
		std::unordered_set<std::pair<const void*, const void*>, PairHash> reported;

		for (int i = 0; i < numManifolds; ++i)
		{
			btPersistentManifold* manifold = disp->getManifoldByIndexInternal(i);
			const btCollisionObject* a = manifold->getBody0();
			const btCollisionObject* b = manifold->getBody1();

			int numContacts = manifold->getNumContacts();
			if (numContacts == 0) continue;

			const void* pa = static_cast<const void*>(a);
			const void* pb = static_cast<const void*>(b);
			std::pair<const void*, const void*> key = (pa < pb) ? std::make_pair(pa, pb) : std::make_pair(pb, pa);
			if (reported.find(key) != reported.end()) continue;
			reported.insert(key);

			btRigidBody* rbA = const_cast<btRigidBody*>(btRigidBody::upcast(a));
			btRigidBody* rbB = const_cast<btRigidBody*>(btRigidBody::upcast(b));
			if (rbA && rbB)
			{
				// forward opaque handles
				collisionCallback(ToHandle(rbA), ToHandle(rbB));
			}
		}
	}

public:
	void StepSimulation(float deltaTime, int maxSubSteps, float fixedTimeStep)
	{
		if (!dynamicsWorld) return;
		dynamicsWorld->stepSimulation(deltaTime, maxSubSteps, fixedTimeStep);
		ProcessCollisions();
	}
};

// Public Physics methods forward to Impl
void Physics::Init(const glm::vec3& gravity)
{
	s_impl = std::make_unique<Impl>();
	const auto success = s_impl->Initialize(gravity);
	DL_ASSERT(success, Log::Categories::Physics, "Failed to initialize Physics");
}

void Physics::Shutdown()
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	s_impl->Shutdown();
	s_impl.reset();
}

void Physics::StepSimulation(float deltaTime, int maxSubSteps, float fixedTimeStep)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	s_impl->StepSimulation(deltaTime, maxSubSteps, fixedTimeStep);
}

RigidBodyHandle Physics::CreateBoxRigidBody(float mass, const glm::vec3& halfExtents, const glm::vec3& position, const glm::quat& orientation)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	return s_impl->CreateBoxRigidBody(mass, halfExtents, position, orientation);
}

RigidBodyHandle Physics::CreateSphereRigidBody(float mass, float radius, const glm::vec3& position, const glm::quat& orientation)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	return s_impl->CreateSphereRigidBody(mass, radius, position, orientation);
}

void Physics::RemoveRigidBody(RigidBodyHandle body)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	s_impl->RemoveRigidBody(body);
}

void Physics::SetGravity(const glm::vec3& gravity)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	s_impl->SetGravity(gravity);
}

bool Physics::GetRigidBodyTransform(RigidBodyHandle body, glm::vec3& outPosition, glm::quat& outOrientation)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	return s_impl->GetRigidBodyTransform(body, outPosition, outOrientation);
}

bool Physics::SetRigidBodyTransform(RigidBodyHandle body, const glm::vec3& position, const glm::quat& orientation)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	return s_impl->SetRigidBodyTransform(body, position, orientation);
}

void Physics::RegisterCollisionCallback(CollisionCallback cb)
{
	DL_ASSERT(s_impl != nullptr, Log::Categories::Physics, "Physics does not exist");
	s_impl->RegisterCollisionCallback(std::move(cb));
}