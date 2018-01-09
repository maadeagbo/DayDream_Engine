#pragma once

#include "ddModel.h"

/**
 * \brief Container for manipulating transforms
 */
struct ddBody {
  /**
   * \brief Assigned on agent creation. Deleted on agent destruction
   */
  btRigidBody* bt_bod;
  /**
   * \brief Initialized on mesh import or set to default size (contains scale)
   */
  btCollisionShape* bt_bbox;
};

namespace ddBodyFuncs {
/**
 * \brief Forward direction based on world (0, 0, -1)
 */
glm::vec3 ForwardDir();
/**
 * \brief Right direction based on world (0, 0, -1)
 */
glm::vec3 RightDir();
/**
 * \brief Up direction based on world (0, 0, -1)
 */
glm::vec3 UpDir();
/**
 * \brief Change btRigidBody's velocity
 */
void UpdateVelocity(const glm::vec3& vel);
/**
 * \brief Change btRigidBody's position
 */
void UpdatePosition(const glm::vec3& pos);
/**
 * \brief Change btRigidBody's rotation
 */
void UpdateRotation(const glm::vec3& rot);
/**
 * \brief Change transform's scale
 */
void UpdateScale(const glm::vec3& _scale);
/**
 * \brief convert btRigidBody and scale into model matrix
 */
glm::mat4 getModelMat();
}

/**
 * \brief Container for instance manipulation
 */
struct ddInstInfo {
  /**
   * \brief gpu instance buffer
   */
  ddInstBufferData* inst_buff;
  /**
   * \brief instanced color buffer
   */
  dd_array<glm::vec3> inst_v3;
  /**
   * \brief Transform info
   */
  dd_array<ddBody> body;
};

/**
 * brief Container for Render settings and buffers
 */
struct ddRendInfo {
  /**
   * \brief matrix buffer for frame manipulation
   */
  dd_array<glm::mat4> f_m4x4;
  /**
   * \brief color buffer for frame manipulation
   */
  dd_array<glm::vec3> f_v3;
  /**
   * \brief indices into matrix buffer for frame manipulation
   */
  dd_array<unsigned> f_cull;
  /**
   * \brief per frame global pose matrix for rendering
   */
  dd_array<glm::mat4> global_pose;
  /**
   * \brief inverse bind pose matrices for rendering
   */
  dd_array<glm::mat4> inv_bp;
  /**
   * \brief Number of instances to be rendered (per frame)
   */
  unsigned f_num_inst = 0;
  /**
   * \brief mark if color instances are present
   */
  bool flag_color = false;
  /**
   * \brief True = render, false = no render
   */
  bool flag_render = true;
};

/**
 * \brief Represents agents w/ in engine
 */
struct ddAgent {
  ddAgent();
  ~ddAgent();

  /**
   * \brief Engine identifier assigned at initialization
   */
  size_t id;
  /**
   * \brief Engine info for scene graph
   */
  ParentInfo parent;
  /**
   * \brief meshes attached to agent. Simulates LOD
   */
  dd_array<ModelIDs> mesh;
  /**
   * \brief Instance information
   */
  ddInstInfo inst;
  /**
   * \brief Render information
   */
  ddRendInfo rend;
};
