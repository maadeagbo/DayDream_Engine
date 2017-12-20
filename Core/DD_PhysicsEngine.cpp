#include "DD_PhysicsEngine.h"

DD_Physics::DD_Physics() {}

DD_Physics::~DD_Physics() {}

void DD_Physics::initialize_world() {
	collision_config = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collision_config);
	broad_phase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	// create world
	world = new btDiscreteDynamicsWorld(dispatcher, broad_phase, solver,
																			collision_config);
	// set gravity
	world->setGravity(btVector3(0, -9.81f, 0));
}

void DD_Physics::cleanup_world() {
	// remove active bodies
	clear_all_rigidbodies();
	// delete world and its components
	delete world;
	delete solver;
	delete broad_phase;
	delete dispatcher;
	delete collision_config;
}

void DD_Physics::clear_all_rigidbodies() {
	// remove any remaining rigid bodies from world and delete
	for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) {
			delete body->getMotionState();
		}
		world->removeCollisionObject(obj);
		delete obj;
	}
}

void DD_Physics::step_simulate(const float dt) {
	const float check = (dt / (1.f / 60.f));
	const unsigned num_iterations = check < 0.f ? 1 : (unsigned)check;

	for (unsigned i = 0; i < num_iterations; i++) {
		world->stepSimulation(1.f / 60.f);
	}
}
