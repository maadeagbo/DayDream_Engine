#pragma once

#include "ddIncludes.h"

#ifdef _MSC_VER
#pragma warning(push, 0)  // disable warnings (low level)
#endif

// Bullet Physics include
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#ifdef _MSC_VER
#pragma warning(pop)  // enable (high level) warning back
#endif

class ddPhysics {
 public:
  ddPhysics();
  ~ddPhysics();

  /**
   * \brief Initializes the necessary modules for Bullet Physics
   */
  void initialize_world();
  /**
   * \brief Close Bullet Physics world
   */
  void cleanup_world();
  /**
   * \brief Remove all rigid bodies from world
   */
  void clear_all_rigidbodies();
  /**
   * \brief Step thru world simulation at 1/60th of a second
   * \param dt Time of last frame
   */
  void step_simulate(const float dt);

  /**
   * \brief Bullet physics world
   */
  btDiscreteDynamicsWorld *world;

 private:
  btDefaultCollisionConfiguration *collision_config;
  btCollisionDispatcher *dispatcher;
  btBroadphaseInterface *broad_phase;
  btSequentialImpulseConstraintSolver *solver;
};