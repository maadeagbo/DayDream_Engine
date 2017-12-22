#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

#include "DD_Types.h"

/**
        Struct for 6-sided frustum
*/
struct FrustumBox {
  glm::vec3 points[6];
  glm::vec3 normals[6];
  glm::vec3 corners[8];
  GLfloat d[6];
};

/**
        Camera class implements FPS-style camera with quaternion rotations. Can
        handle VR applications as well.
*/
class DD_Camera {
 public:
  DD_Camera(const GLfloat screenW = 0.0f, const GLfloat screenH = 0.0f,
            const GLfloat fovh = 45.0f, const GLfloat nearP = 1.0f,
            const GLfloat farP = 10000.0f)
      : scr_width(screenW),
        scr_height(screenH),
        fov_h(glm::radians(fovh)),
        near_plane(nearP),
        far_plane(farP),
        active(false),
        vr_cam(false),
        vr_window(false),
        world_up(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)),
        cam_pos(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)),
        cam_quat(glm::quat()),
        cam_front(glm::vec3(0.0f, 0.0f, -1.0f)),
        cam_up(glm::vec3(0.0f, 1.0f, 0.0f)),
        _flag_parent(false) {}

  inline glm::vec4 pos() const { return cam_pos; }
  inline glm::quat camQuat() const { return cam_quat; }
  inline glm::vec4 worldUp() const { return world_up; }
  inline glm::vec3 front() const { return cam_front; }
  inline FrustumBox frustum() const { return cam_frustum; }
  void updateCamera(const glm::vec4 pos, const glm::quat quaternion);
  void SetupFrustum();
  inline void SetParent(const char* parentID) {
    _parent_id = parentID;
    _parent_idx = -1;
    _flag_parent = true;
  }
  inline void unParent() {
    _parent_id = "";
    _parent_idx = -1;
    _flag_parent = false;
  }
  inline void SetParentIndex(const int index) { _parent_idx = index; }
  inline int parentIndex() const { return _parent_idx; }
  inline std::string parentID() const { return _parent_id; }
  inline bool isChild() const { return _flag_parent; }

  std::string m_ID;
  glm::mat4 parent_transform;
  glm::vec3 vr_scr_pos;
  glm::vec3 vr_eye_pos;
  GLfloat scr_width;
  GLfloat scr_height;
  GLfloat fov_h;
  GLfloat near_plane;
  GLfloat far_plane;
  GLfloat cam_eye_dist;
  bool active;
  bool vr_cam;
  bool vr_window;

 private:
  std::string _parent_id;
  FrustumBox cam_frustum;
  glm::vec4 world_up;
  glm::vec4 cam_pos;
  glm::quat cam_quat;
  glm::vec3 cam_front;
  glm::vec3 cam_up;
  glm::vec3 cam_right;
  int _parent_idx;
  GLboolean _flag_parent;
};

namespace CamSpace {
glm::mat4 GetViewMatrix(const DD_Camera* cam);
glm::mat4 GetViewMatrixVREye(const DD_Camera* cam, const bool _left,
                             const bool _right, const float dist);
glm::mat4 GetPerspecProjMatrix(const DD_Camera* cam);
glm::mat4 GetOffAxisProjMatrix(const DD_Camera* cam, const glm::vec3 screenPos,
                               const float scrWidth, const float scrHeight,
                               const glm::vec3 eyePos);
void PrintInfo(const DD_Camera& cam);
}

/// \brief Describes camera attributes
struct DD_Cam {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Engine info for scene graph
  ParentInfo parent;
  /// \brief Horizontal field of view
  float fovh = 0.f;
  /// \brief Near plane
  float n_plane = 0.f;
  /// \brief Far plane
  float f_plane = 100.f;
  /// \brief VR camera left/right eye distance in meters
  float eye_dist = 0.f;
  /// \brief Frustum information. Set up at initialization (position: 0, 0, 0)
  FrustumBox cam_frustum;

  /// \brief Marks instance as active camera
  bool active = false;
  /// \brief Marks camera as stereoscopic
  bool vr_flag = false;
};