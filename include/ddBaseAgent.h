#pragma once

#include "LuaHooks.h"
#include "ddModel.h"
#include "ddPhysicsEngine.h"
#include "ddSkeleton.h"

/** \brief Container for transform information */
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
  /**
   * \brief Finer bounding box approximation structure
   */
  dd_array<OOBoundingBox> oobbs;
};

/** \brief Interace for bullet physics transforms */
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

/** \brief Container for Render settings and buffers */
struct ddAnimInfo {
  /**
   * \brief skeleton id
   */
  size_t sk_id = 0;
  /**
   * \brief per frame global pose matrixcv for rendering
   */
  dd_array<glm::mat4> global_pose;
  /**
   * \brief inverse bind pose matrices for rendering
   */
  dd_array<glm::mat4> inv_bp;
  /**
   * \brief per-frame calculated local pose data
   */
  dd_array<ddJointPose> local_pose;
  /**
   * \brief ddAnimState container
   */
  dd_array<ddAnimState> states;
  /**
   * \brief Marks how to finalize local to global pose data
   */
  bool global_calc = false;
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
  /**
   * \brief Animation info
   */
  ddAnimInfo anim;
};

/** \brief Lua class instance metatable name */
const char *ddAgent_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddAgent(lua_State *L);

/** \brief Lua class instance metatable name */
const char *ddAnimState_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddAnimState(lua_State *L);
