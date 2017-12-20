#pragma once

#include "DD_Types.h"

class DD_Physics {
public:
	DD_Physics();
	~DD_Physics();

	/// \brief Initializes the necessary modules for Bullet Physics
	void initialize_world();
	/// \brief Close Bullet Physics world
	void cleanup_world();
	/// \brief Remove all rigid bodies from world
	void clear_all_rigidbodies();
	/// \brief Step thru world simulation at 1/60th of a second
	/// \param dt Time of last frame
	void step_simulate(const float dt);

	/// \brief Bullet physics world
	btDiscreteDynamicsWorld *world;
private:
	btDefaultCollisionConfiguration *collision_config;
	btCollisionDispatcher *dispatcher;
	btBroadphaseInterface *broad_phase;
	btSequentialImpulseConstraintSolver *solver;
};