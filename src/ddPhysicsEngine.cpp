#include "ddPhysicsEngine.h"

ddPhysics::ddPhysics() {}

ddPhysics::~ddPhysics() {}

void ddPhysics::initialize_world() {
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

void ddPhysics::cleanup_world() {
  // remove active bodies
  clear_all_rigidbodies();
  // delete world and its components
  delete world;
  world = nullptr;
  delete solver;
  solver = nullptr;
  delete broad_phase;
  broad_phase = nullptr;
  delete dispatcher;
  dispatcher = nullptr;
  delete collision_config;
  collision_config = nullptr;
}

void ddPhysics::clear_all_rigidbodies() {
  // remove any remaining rigid bodies from world and delete
  for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
    btCollisionObject *obj = world->getCollisionObjectArray()[i];
    btRigidBody *body = btRigidBody::upcast(obj);
    if (body && body->getMotionState()) {
      delete body->getMotionState();
    }
    delete body->getCollisionShape();
    world->removeCollisionObject(obj);
    delete obj;
  }
  // delete constraints attached to rigid bodies
  for (int i = world->getNumConstraints() - 1; i >= 0; i--) {
    btTypedConstraint *constraint = world->getConstraint(i);
    world->removeConstraint(constraint);
    delete constraint;
  }
}

void ddPhysics::step_simulate(const float dt) { world->stepSimulation(dt); }
