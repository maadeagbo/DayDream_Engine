#pragma once

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

/*-----------------------------------------------------------------------------
*
*	DD_Model:
*		- Store 3D mesh information for rendering
*			- OpenGL VAO info
*			- Material info
*			- Level of detail meshes
*			- Bounding Box
*	Model(namespace):
*		- loadModel
*			-
*		- CalculateBoundingBox
*			-
*		- OpenGLBindMesh
*			-
*	TODO:
*
-----------------------------------------------------------------------------*/

#include "GPUFrontEnd.h"
#include "LuaHooks.h"
#include "ddIncludes.h"

// struct DD_Model {
//   std::string m_ID;
//   dd_array<DDM_Data> meshes;
//   dd_array<GLuint> VAO, VBO, EBO, instVBO, instColorVBO;
//   dd_array<size_t> materials;  // index
//   std::string directory;
//   bool m_loaded_to_GPU = false;
// };

// namespace ModelSpace {
// // BoundingBox CalculateBBox(const DD_Model& model);
// void OpenGLBindMesh(const int index, DD_Model& model, const size_t inst_size,
//                     const size_t inst_c_size);
// void OpenGLUnBindMesh(const int index, DD_Model& model);
// // void PrintInfo(const DD_Model& mod);
// }

struct LinePoint {
  glm::vec4 pos01, pos02;
};

struct DD_LineAgent {
  dd_array<LinePoint> lines;
  dd_array<glm::vec4> m_buffer;
  glm::vec4 color = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
  std::string m_ID = "";
  bool flag_render = true, m_flagXZ = true;
  DD_LineAgent(const size_t size = 0) : lines(size) {}
  inline void FlushLines() {
    lines.resize(0);
    m_buffer.resize(0);
  }
};

/** \brief Bounding box representation */
struct BoundingBox {
  BoundingBox() {}
  BoundingBox(const glm::vec3 _min, const glm::vec3 _max)
      : min(_min), max(_max) {
    SetCorners();
  }

  glm::vec3 min, max;
  glm::vec3 corner1, corner2, corner3, corner4, corner5, corner6, corner7,
      corner8;
  dd_array<glm::vec4> buffer = dd_array<glm::vec4>(12 * 2);

  void SetCorners();
  BoundingBox transformCorners(const glm::mat4 transMat) const;
  glm::vec3 UpdateAABB_min();
  glm::vec3 UpdateAABB_max();
  glm::vec3 GetFrustumPlaneMin(glm::vec3 norm);
  glm::vec3 GetFrustumPlaneMax(glm::vec3 norm);
  void SetLineBuffer();
};

/** \brief Object-oriented bounding box */
struct OOBoundingBox {
  glm::quat rot;
  glm::vec3 pos;
  glm::vec3 scale;
  glm::vec3 mirror;
  int joint_idx = -1;
  bool mirror_flag = false;
  const BoundingBox get_bbox() const;
};

// Useful glm functions
glm::vec4 getVec4f(const char *str);

glm::uvec4 getVec4u(const char *str);

glm::vec3 getVec3f(const char *str);

glm::vec2 getVec2f(const char *str);

glm::quat getQuat(const char *str);

std::string Vec4f_Str(const glm::vec4 vIn);

glm::mat4 createMatrix(const glm::vec3 &pos, const glm::vec3 &rot,
                       const glm::vec3 &scale);

void printGlmMat(glm::mat4 mat);

/// \brief Container for agent mesh information
struct ModelIDs {
  /// \brief LOD bounds
  float _near = 0.f, _far = 100.f;
  /// \brief Mesh id
  size_t model;
  /// \brief Material id
  dd_array<size_t> material;
  /// \brief Handles for gpu object data
  dd_array<ddVAOData *> vao_handles;
  /// \brief Marks whether the mesh is skinned for animation
  bool sk_flag = false;
  /// \brief Marks if model should cast shadow
  bool cast_shadow = true;
};

/// \brief Data used by render engine for drawing
struct ddModelData {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Mesh information
  dd_array<DDM_Data> mesh_info;
  /// \brief GPU buffer handles
  dd_array<ddMeshBufferData *> buffers;
};

/** \brief Lua class instance metatable name */
const char *ddModelData_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddModelData(lua_State *L);
