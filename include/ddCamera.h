#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

#include "LuaHooks.h"
#include "ddIncludes.h"

/**
 * \brief Struct for 6-sided frustum
*/
struct FrustumBox {
  glm::vec3 points[6];
  glm::vec3 normals[6];
  glm::vec3 corners[8];
  float d[6];
};

// namespace CamSpace {
// glm::mat4 GetViewMatrix(const ddCamera* cam);
// glm::mat4 GetViewMatrixVREye(const ddCamera* cam, const bool _left,
//                              const bool _right, const float dist);
// glm::mat4 GetPerspecProjMatrix(const ddCamera* cam);
// glm::mat4 GetOffAxisProjMatrix(const ddCamera* cam, const glm::vec3
// screenPos,
//                                const float scrWidth, const float scrHeight,
//                                const glm::vec3 eyePos);
// void PrintInfo(const ddCamera& cam);
// }

/**
 * \brief Describes camera attributes
 */
struct ddCam {
  /**
   * \brief Engine identifier assigned at initialization
   */
  size_t id;
  /**
   * \brief Object camera is attached to
   */
  size_t parent;
  /**
   * \brief Horizontal field of view
   */
  float fovh = 0.f;
  /**
   * \brief Near plane
   */
  float n_plane = 0.f;
  /**
   * \brief Far plane
   */
  float f_plane = 100.f;
  /**
   * \brief VR camera left/right eye distance in meters
   */
  float eye_dist = 0.f;
  /**
* \brief Camera's rotation
*/
  float roll = 0.f, pitch = 0.f, yaw = 0.f;
  /**
   * \brief VR camera left/right eye distance in meters
   */
  unsigned width = 0, height = 0;
  /**
   * \brief Frustum information. Set up at initialization (position: 0, 0, 0)
   */
  FrustumBox cam_frustum;

  /**
   * \brief Marks instance as active camera
   */
  bool active = false;
  /**
   * \brief Marks camera as stereoscopic
   */
  bool vr_flag = false;
};

/** \brief Lua class instance metatable name */
const char* ddCam_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddCam(lua_State* L);