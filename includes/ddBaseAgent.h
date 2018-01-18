#pragma once

#include "ddModel.h"

/** \brief Container for manipulating transforms */
struct ddBody {
  /**
   * \brief Assigned on agent creation. Deleted on agent destruction
   */
  btRigidBody *bt_bod = nullptr;
  /**
   * \brief Constraint for parenting
   */
	btGeneric6DofSpring2Constraint *bt_constraint = nullptr;
	/**
   * \brief Parent object
   */
	size_t parent = 0;
	/**
   * \brief Parent-child object offset
   */
	glm::vec3 offset;
  /**
* \brief Modify object scale (and affect physics using btCollisionShape)
*/
  glm::vec3 scale = glm::vec3(1.f);
};

namespace ddBodyFuncs {
/** \brief Simple axis-alligned bounding box */
struct AABB {
  glm::vec3 min, max;
};
/**
 * \brief Local-space position
 */
glm::vec3 pos(const ddBody *bod);
/**
 * \brief World-space position
 */
glm::vec3 pos_ws(const ddBody *bod);
/**
 * \brief Local-space rotation (euler)
 */
glm::quat rot(const ddBody *bod);
/**
 * \brief World-space rotation (euler)
 */
glm::quat rot_ws(const ddBody *bod);
/**
 * \brief Forward direction based on world (0, 0, -1)
 */
glm::vec3 forward_dir(const ddBody *bod);
/**
 * \brief Change btRigidBody's velocity
 */
void update_velocity(ddBody *bod, const glm::vec3 &vel);
/**
 * \brief Change btRigidBody's position
 */
void update_pos(ddBody *bod, const glm::vec3 &pos);
/**
 * \brief Change btRigidBody's local rotation
 */
void rotate(ddBody *bod, const float yaw, const float pitch, const float roll);
/**
 * \brief Change transform's scale
 */
void update_scale(ddBody *bod, const glm::vec3 &_scale);
/**
 * \brief convert btRigidBody and scale into model matrix (uses world-space)
 */
glm::mat4 get_model_mat(ddBody *bod);
/**
 * \brief Get AABB from btRigidBody
 */
AABB get_aabb(ddBody *bod);
}

/** \brief Container for instance manipulation */
struct ddInstInfo {
  /**
   * \brief gpu instance buffer
   */
  ddInstBufferData *inst_buff = nullptr;
  /**
   * \brief instanced matrix buffer
   */
  dd_array<glm::mat4> m4x4 = dd_array<glm::mat4>(1);
  /**
   * \brief instanced color buffer
   */
  dd_array<glm::vec3> v3 = dd_array<glm::vec3>(1);
};

/** \brief Container for Render settings and buffers */
struct ddRendInfo {
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
  /**
  * \brief Transform info
  */
  ddBody body;
};
