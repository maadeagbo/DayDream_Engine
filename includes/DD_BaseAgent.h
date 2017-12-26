#pragma once

#include "DD_Model.h"

/// \brief Container for instance manipulation
struct DD_InstInfo {
  /// \brief model space buffer
  dd_array<glm::mat4> inst_m4x4;
  /// \brief color modification buffer
  dd_array<glm::vec3> inst_v3;
  /// \brief physics system buffer
  dd_array<btRigidBody*> inst_body;
};

/// brief Container for Render settings and buffers
struct DD_RendInfo {
  /// \brief gpu buffer handles
  GPUInfo buffer_info;
  /// \brief matrix buffer for frame manipulation
  dd_array<glm::mat4> f_m4x4;
  /// \brief color buffer for frame manipulation
  dd_array<glm::vec3> f_v3;
  /// \brief indices into matrix buffer for frame manipulation
  dd_array<unsigned> f_cull;
  /// \brief per frame global pose matrix for rendering
  dd_array<glm::mat4> global_pose;
  /// \brief inverse bind pose matrices for rendering
  dd_array<glm::mat4> inv_bp;
  /// \brief Number of instances to be rendered (per frame)
  unsigned f_num_inst = 0;
  /// \brief mark if color instances are present
  bool flag_color = false;
  /// \brief True = render, false = no render
  bool flag_render = true;
};

/// \brief Represents agents w/ in engine
struct DD_BaseAgent {
  DD_BaseAgent();
  ~DD_BaseAgent();

  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Engine info for scene graph
  ParentInfo parent;
  /// \brief meshes attached to agent. Also simulates LODs
  dd_array<ModelIDs> mesh;
  /// \brief Transform attached to agent
  DD_Body body;
  /// \brief Instance information
  DD_InstInfo inst_info;
  /// \brief Render information
  DD_RendInfo rend_info;
};
